#include "localized_event.h"

#include <string>

#include "creature.h"
#include "item.h"
#include "mapdata.h"
#include "monster.h"
#include "mtype.h"
#include "recipe.h"

#if defined(LOCALIZE)

#include <unicode/fmtable.h>
#include <unicode/messagepattern.h>
#include <unicode/msgfmt.h>
#include <unicode/numberformatter.h>
#include <unicode/plurrule.h>
#include <unicode/stringpiece.h>
#include <unicode/unistr.h>

#include <algorithm>
#include <initializer_list>
#include <map>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "translation_cache.h"
#include "translation_manager.h"
#include "translations.h"

namespace
{
struct Argument {
    std::string role;
    std::variant<const Creature *, bodypart_id, const map_data_common_t *, const item *,
        const recipe *, int, std::string> value;
};

struct PreparedMessage {
    std::unique_ptr<icu::MessageFormat> formatter;
    std::vector<std::string> names;
};

std::map<std::string, PreparedMessage> &message_cache()
{
    static thread_local int version = INVALID_LANGUAGE_VERSION;
    static thread_local std::map<std::string, PreparedMessage> cache;
    if( version != detail::get_current_language_version() ) {
        cache.clear();
        version = detail::get_current_language_version();
    }
    return cache;
}

PreparedMessage *prepare_message( const std::string &pattern, const std::string &language )
{
    auto &cache = message_cache();
    const std::string key = language + '\n' + pattern;
    const auto found = cache.find( key );
    if( found != cache.end() ) {
        return &found->second;
    }

    UErrorCode status = U_ZERO_ERROR;
    const icu::UnicodeString text = icu::UnicodeString::fromUTF8( pattern );
    icu::MessagePattern parsed( status );
    parsed.parse( text, nullptr, status );
    if( U_FAILURE( status ) ) {
        return nullptr;
    }
    PreparedMessage prepared;
    for( int32_t part = 0; part < parsed.countParts(); ++part ) {
        if( parsed.getPartType( part ) == UMSGPAT_PART_TYPE_ARG_NAME ) {
            std::string name;
            parsed.getSubstring( parsed.getPart( part ) ).toUTF8String( name );
            if( std::find( prepared.names.begin(), prepared.names.end(), name ) ==
                prepared.names.end() ) {
                prepared.names.push_back( std::move( name ) );
            }
        }
    }
    prepared.formatter = std::make_unique<icu::MessageFormat>( text,
                         icu::Locale( language.c_str() ), status );
    if( U_FAILURE( status ) ) {
        return nullptr;
    }
    return &cache.emplace( key, std::move( prepared ) ).first->second;
}

std::string uppercase_initial( const std::string &value, const std::string &language )
{
    icu::UnicodeString text = icu::UnicodeString::fromUTF8( value );
    if( text.isEmpty() ) {
        return value;
    }
    const int32_t first_end = text.moveIndex32( 0, 1 );
    icu::UnicodeString initial = text.tempSubStringBetween( 0, first_end );
    initial.toUpper( icu::Locale( language.c_str() ) );
    text.replaceBetween( 0, first_end, initial );
    std::string result;
    text.toUTF8String( result );
    return result;
}

std::optional<std::string> creature_name( const Creature &creature, const std::string &form,
        const std::string &language )
{
    const bool english = language == "en";
    if( const monster *mon = creature.as_monster() ) {
        if( !mon->type ) {
            return std::nullopt;
        }
        if( !english && mon->nickname.empty() && !mon->type->has_localized_name() ) {
            return std::nullopt;
        }
        const std::string base = english ? mon->source_name() : mon->name();
        if( base.empty() ) {
            return std::nullopt;
        }
        if( form == "nomcap" ) {
            if( english && mon->nickname.empty() ) {
                return "The " + base;
            }
            return uppercase_initial( base, language );
        }
        if( form == "nom" ) {
            return english && mon->nickname.empty() ? "the " + base : base;
        }
        if( form == "acc" ) {
            if( english ) {
                return mon->nickname.empty() ? "the " + base : base;
            }
            const translation &accusative = mon->type->accusative_name();
            if( mon->has_custom_name() || !accusative.has_translation() ) {
                return std::nullopt;
            }
            return accusative.translated();
        }
        if( form == "gen" ) {
            if( english ) {
                return mon->nickname.empty() ? "the " + base + "'s" : base + "'s";
            }
            const translation &genitive = mon->type->genitive_name();
            if( mon->has_custom_name() || !genitive.has_translation() ) {
                return std::nullopt;
            }
            return genitive.translated();
        }
        if( form == "label" ) {
            return base;
        }
        return std::nullopt;
    }
    if( form == "nom" || form == "nomcap" || form == "label" ||
        ( english && ( form == "acc" || form == "gen" ) ) ) {
        const std::string name = creature.get_name();
        if( form == "gen" ) {
            return name + "'s";
        }
        return form == "nomcap" ? uppercase_initial( name, language ) : name;
    }
    return std::nullopt;
}

std::optional<std::string> resolve_string( const Argument &argument, const std::string &form,
        const std::string &language )
{
    const bool english = language == "en";
    if( const auto creature = std::get_if<const Creature *>( &argument.value ) ) {
        return creature_name( **creature, form, language );
    }
    if( const auto body = std::get_if<bodypart_id>( &argument.value ) ) {
        if( form == "acc" ) {
            const translation &name = ( **body ).accusative;
            if( !english && !name.has_translation() ) {
                return std::nullopt;
            }
            return english ? name.source_text() : name.translated();
        }
        if( form == "nom" ) {
            const translation &name = ( **body ).name;
            if( !english && !name.has_translation() ) {
                return std::nullopt;
            }
            return english ? name.source_text() : name.translated();
        }
        return std::nullopt;
    }
    if( const auto surface = std::get_if<const map_data_common_t *>( &argument.value ) ) {
        if( form == "relation" ) {
            return ( *surface )->impact_relation();
        }
        if( form == "acc" ) {
            const translation &name = ( *surface )->accusative_name();
            if( !english && !name.has_translation() ) {
                return std::nullopt;
            }
            return english ? "the " + ( *surface )->source_name() : name.translated();
        }
        if( form == "nom" ) {
            if( !english && !( *surface )->has_localized_name() ) {
                return std::nullopt;
            }
            return english ? ( *surface )->source_name() : ( *surface )->name();
        }
    }
    if( const auto object = std::get_if<const item *>( &argument.value ) ) {
        if( form != "label" || ( english &&
                                 TranslationManager::GetInstance().GetCurrentLanguage() != "en" ) ) {
            return std::nullopt;
        }
        return ( *object )->tname();
    }
    if( const auto product = std::get_if<const recipe *>( &argument.value ) ) {
        if( form != "label" || ( english &&
                                 TranslationManager::GetInstance().GetCurrentLanguage() != "en" ) ) {
            return std::nullopt;
        }
        return ( *product )->result_name();
    }
    if( const auto decimal = std::get_if<std::string>( &argument.value ) ) {
        const std::optional<localized_quantity> quantity = format_localized_quantity( *decimal, language );
        if( !quantity ) {
            return std::nullopt;
        }
        if( form == "display" ) {
            return quantity->display;
        }
        if( form == "category" ) {
            return quantity->category;
        }
    }
    return std::nullopt;
}

std::optional<std::string> render_message( const std::string &pattern,
        const std::string &language, std::initializer_list<Argument> arguments )
{
    PreparedMessage *prepared = prepare_message( pattern, language );
    if( !prepared ) {
        return std::nullopt;
    }
    std::vector<icu::UnicodeString> names;
    std::vector<icu::Formattable> values;
    names.reserve( prepared->names.size() );
    values.reserve( prepared->names.size() );
    for( const std::string &name : prepared->names ) {
        const std::size_t separator = name.rfind( '_' );
        const std::string role = name.substr( 0, separator );
        const std::string form = separator == std::string::npos ? "" : name.substr( separator + 1 );
        const auto found = std::find_if( arguments.begin(), arguments.end(),
        [&]( const Argument & arg ) {
            return arg.role == role;
        } );
        if( found == arguments.end() ) {
            return std::nullopt;
        }
        names.push_back( icu::UnicodeString::fromUTF8( name ) );
        if( const int *number = std::get_if<int>( &found->value ) ) {
            if( !form.empty() ) {
                return std::nullopt;
            }
            values.emplace_back( *number );
        } else {
            const std::optional<std::string> value = resolve_string( *found, form, language );
            if( !value || value->empty() ) {
                return std::nullopt;
            }
            values.emplace_back( icu::UnicodeString::fromUTF8( *value ) );
        }
    }
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString result;
    prepared->formatter->format( names.data(), values.data(),
                                 static_cast<int32_t>( names.size() ), result, status );
    if( U_FAILURE( status ) ) {
        return std::nullopt;
    }
    std::string output;
    result.toUTF8String( output );
    return output;
}

std::string format_event( const char *context, const char *source,
                          std::initializer_list<Argument> arguments, const char *english_fallback )
{
    const std::string language = TranslationManager::GetInstance().GetCurrentLanguage();
    if( language != "en" && TranslationManager::GetInstance().HasTranslation( context, source ) ) {
        if( auto translated = render_message( pgettext( context, source ), language, arguments ) ) {
            return *translated;
        }
    }
    return render_message( source, "en", arguments ).value_or( english_fallback );
}
} // namespace

std::optional<localized_quantity> format_localized_quantity( const std::string &decimal,
        const std::string &language )
{
    const std::size_t point = decimal.find( '.' );
    const std::size_t digits = point == std::string::npos ? 0 : decimal.size() - point - 1;
    if( decimal.empty() || digits > 9 ) {
        return std::nullopt;
    }
    const icu::Locale locale( language.c_str() );
    UErrorCode status = U_ZERO_ERROR;
    const auto formatter = icu::number::NumberFormatter::withLocale( locale ).precision(
                               icu::number::Precision::fixedFraction( static_cast<int32_t>( digits ) ) );
    const auto formatted = formatter.formatDecimal( icu::StringPiece( decimal ), status );
    std::unique_ptr<icu::PluralRules> rules( icu::PluralRules::forLocale( locale, status ) );
    if( U_FAILURE( status ) || !rules ) {
        return std::nullopt;
    }
    const icu::UnicodeString category = rules->select( formatted, status );
    const icu::UnicodeString display = formatted.toString( status );
    if( U_FAILURE( status ) ) {
        return std::nullopt;
    }
    localized_quantity result;
    display.toUTF8String( result.display );
    category.toUTF8String( result.category );
    return result;
}

std::optional<std::string> format_bite_event( const monster &attacker, const Creature &target,
        const bodypart_id &body, bite_event_result result )
{
    const bool player = target.is_avatar();
    const char *context;
    const char *source;
    if( result == bite_event_result::miss ) {
        context = player ? "rodina.event.bite.player.miss" : "rodina.event.bite.other.miss";
        source = player ?
                 translate_marker_context( "rodina.event.bite.player.miss",
                                           "{attacker_nomcap} lunges at you, but you dodge!" ) :
                 translate_marker_context( "rodina.event.bite.other.miss",
                                           "{attacker_nomcap} lunges at {target_acc}, but misses!" );
    } else if( result == bite_event_result::no_damage ) {
        context = player ? "rodina.event.bite.player.no_damage" :
                  "rodina.event.bite.other.no_damage";
        source = player ?
                 translate_marker_context( "rodina.event.bite.player.no_damage",
                                           "{attacker_nomcap} bites your {body_acc}, but fails to penetrate armor!" ) :
                 translate_marker_context( "rodina.event.bite.other.no_damage",
                                           "{attacker_nomcap} bites {target_acc} in the {body_acc}, but fails to penetrate armor!" );
    } else {
        context = player ? "rodina.event.bite.player.hit" : "rodina.event.bite.other.hit";
        source = player ?
                 translate_marker_context( "rodina.event.bite.player.hit",
                                           "{attacker_nomcap} bites your {body_acc}!" ) :
                 translate_marker_context( "rodina.event.bite.other.hit",
                                           "{attacker_nomcap} bites {target_acc} in the {body_acc}!" );
    }
    const char *fallback = result == bite_event_result::miss ?
                           ( player ? "A creature lunges at you, but misses." :
                             "A creature lunges at another creature, but misses." ) :
                           result == bite_event_result::no_damage ?
                           ( player ? "A creature bites you, but does no damage." :
                             "A creature bites another creature, but does no damage." ) :
                           ( player ? "A creature bites you." : "A creature bites another creature." );
    return format_event( context, source, { { "attacker", &attacker }, { "target", &target },
        { "body", body }
    }, fallback );
}

std::optional<std::string> format_impact_event( const map_data_common_t &surface,
        impact_event_result result, int damage )
{
    if( surface.impact_relation() != "ground" && surface.impact_relation() != "barrier" ) {
        return std::nullopt;
    }
    const char *context;
    const char *source;
    const char *fallback;
    switch( result ) {
        case impact_event_result::landing:
            context = "rodina.event.impact.landing";
            source = translate_marker_context( "rodina.event.impact.landing",
                                               "You land on {surface_acc}." );
            fallback = "You land on a surface.";
            break;
        case impact_event_result::landing_damage:
            context = "rodina.event.impact.landing_damage";
            source = translate_marker_context( "rodina.event.impact.landing_damage",
                                               "You land on {surface_acc} and take {damage} damage." );
            fallback = "You land on a surface and take damage.";
            break;
        case impact_event_result::landing_shock_absorbed:
            context = "rodina.event.impact.landing_shock_absorbed";
            source = translate_marker_context( "rodina.event.impact.landing_shock_absorbed",
                                               "You land on {surface_acc}, but your shock absorbers negate the damage!" );
            fallback = "You land on a surface, but your shock absorbers negate the damage.";
            break;
        case impact_event_result::slam_no_damage:
            context = "rodina.event.impact.slam_no_damage";
            source = translate_marker_context( "rodina.event.impact.slam_no_damage",
                                               "{surface_relation, select, ground {You are slammed onto {surface_acc}, but take no damage.} barrier {You are slammed against {surface_acc}, but take no damage.} other {You are slammed against {surface_acc}, but take no damage.}}" );
            fallback = "You are slammed into an obstacle, but take no damage.";
            break;
        case impact_event_result::slam_shock_absorbed:
            context = "rodina.event.impact.slam_shock_absorbed";
            source = translate_marker_context( "rodina.event.impact.slam_shock_absorbed",
                                               "{surface_relation, select, ground {You are slammed onto {surface_acc}, but your shock absorbers negate the damage!} barrier {You are slammed against {surface_acc}, but your shock absorbers negate the damage!} other {You are slammed against {surface_acc}, but your shock absorbers negate the damage!}}" );
            fallback = "You are slammed into an obstacle, but your shock absorbers negate the damage.";
            break;
        case impact_event_result::slam_damage:
            context = "rodina.event.impact.slam_damage";
            source = translate_marker_context( "rodina.event.impact.slam_damage",
                                               "{surface_relation, select, ground {You are slammed onto {surface_acc} for {damage} damage.} barrier {You are slammed against {surface_acc} for {damage} damage.} other {You are slammed against {surface_acc} for {damage} damage.}}" );
            fallback = "You are slammed into an obstacle and take damage.";
            break;
        default:
            return std::nullopt;
    }
    return format_event( context, source, { { "surface", &surface }, { "damage", damage } }, fallback );
}

std::optional<std::string> format_shot_event( const Creature &attacker, const Creature &target )
{
    const monster *target_mon = target.as_monster();
    if( !target_mon || !target_mon->type || target_mon->type->accusative_name().empty() ) {
        return std::nullopt;
    }
    const char *source = translate_marker_context( "rodina.event.shot.hit",
                         "{attacker_nomcap} shoots {target_acc}." );
    return format_event( "rodina.event.shot.hit", source,
    { { "attacker", &attacker }, { "target", &target } },
    "One creature shoots another creature." );
}

std::optional<std::string> format_dodge_event( const Creature &defender, const monster &attacker )
{
    if( !attacker.type || attacker.type->genitive_name().empty() ) {
        return std::nullopt;
    }
    if( defender.is_avatar() ) {
        const char *source = translate_marker_context( "rodina.event.dodge.player",
                             "You dodge an attack from {attacker_nom}." );
        return format_event( "rodina.event.dodge.player", source,
        { { "attacker", &attacker } }, "You dodge an attack." );
    }
    const char *source = translate_marker_context( "rodina.event.dodge.other",
                         "{defender_nomcap} dodges an attack from {attacker_nom}." );
    return format_event( "rodina.event.dodge.other", source,
    { { "defender", &defender }, { "attacker", &attacker } },
    "A creature dodges an attack." );
}

std::optional<std::string> format_rapid_no_damage_event( const Creature &target )
{
    const monster *target_mon = target.as_monster();
    if( !target_mon || !target_mon->type || target_mon->type->accusative_name().empty() ) {
        return std::nullopt;
    }
    const char *source = translate_marker_context( "rodina.event.rapid.no_damage",
                         "You quickly strike {target_acc}, but deal no damage." );
    return format_event( "rodina.event.rapid.no_damage", source, { { "target", &target } },
    "You strike a creature quickly, but deal no damage." );
}

std::optional<std::string> format_toolmod_event( const item &mod, const item &tool )
{
    const char *source = translate_marker_context( "rodina.event.toolmod.attached",
                         "You successfully attach {mod_label} to {tool_label}." );
    return format_event( "rodina.event.toolmod.attached", source,
    { { "mod", &mod }, { "tool", &tool } },
    "You attach a modification to a tool." );
}

std::optional<std::string> format_craft_event( const recipe &product, bool from_memory )
{
    if( from_memory ) {
        const char *source = translate_marker_context( "rodina.event.craft.memory",
                             "Crafted from memory: {product_label}." );
        return format_event( "rodina.event.craft.memory", source, { { "product", &product } },
        "An item was crafted from memory." );
    }
    const char *source = translate_marker_context( "rodina.event.craft.reference",
                         "Crafted using a reference: {product_label}." );
    return format_event( "rodina.event.craft.reference", source, { { "product", &product } },
    "An item was crafted using a reference." );
}

std::optional<std::string> format_burning_duration_event( int count, bool hours )
{
    if( count < 0 ) {
        return std::nullopt;
    }
    return format_burning_duration_event( std::to_string( count ), hours );
}

std::optional<std::string> format_burning_duration_event( const std::string &decimal, bool hours )
{
    if( decimal.empty() || decimal.front() == '-' || !format_localized_quantity( decimal, "en" ) ) {
        return std::nullopt;
    }
    if( hours ) {
        const char *source = translate_marker_context( "rodina.event.burning.hours",
                             "{count_category, select, one {It will finish burning in about {count_display} hour.} other {It will finish burning in about {count_display} hours.}}" );
        return format_event( "rodina.event.burning.hours", source,
        { { "count", decimal } },
        "It will finish burning in about an hour." );
    }
    const char *source = translate_marker_context( "rodina.event.burning.minutes",
                         "{count_category, select, one {It should take about {count_display} minute to finish burning.} other {It should take about {count_display} minutes to finish burning.}}" );
    return format_event( "rodina.event.burning.minutes", source,
    { { "count", decimal } },
    "It should finish burning soon." );
}

#else

std::optional<std::string> format_bite_event( const monster &, const Creature &,
        const bodypart_id &, bite_event_result )
{
    return std::nullopt;
}

std::optional<std::string> format_impact_event( const map_data_common_t &,
        impact_event_result, int )
{
    return std::nullopt;
}

std::optional<std::string> format_shot_event( const Creature &, const Creature & )
{
    return std::nullopt;
}

std::optional<std::string> format_dodge_event( const Creature &, const monster & )
{
    return std::nullopt;
}

std::optional<std::string> format_rapid_no_damage_event( const Creature & )
{
    return std::nullopt;
}

std::optional<std::string> format_toolmod_event( const item &, const item & )
{
    return std::nullopt;
}

std::optional<std::string> format_craft_event( const recipe &, bool )
{
    return std::nullopt;
}

std::optional<std::string> format_burning_duration_event( int, bool )
{
    return std::nullopt;
}

std::optional<std::string> format_burning_duration_event( const std::string &, bool )
{
    return std::nullopt;
}

std::optional<localized_quantity> format_localized_quantity( const std::string &,
        const std::string & )
{
    return std::nullopt;
}

#endif
