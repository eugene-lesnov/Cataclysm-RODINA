#include <unicode/fmtable.h>
#include <unicode/messagepattern.h>
#include <unicode/msgfmt.h>
#include <unicode/numberformatter.h>
#include <unicode/plurrule.h>
#include <unicode/stringpiece.h>
#include <unicode/unistr.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

extern "C" {
struct FormEntry {
    const char *role;
    const char *form;
    const char *value;
};
void *fluent_catalog_new( const char *locale, const char *resource );
char *fluent_catalog_render( void *catalog, const char *event, const FormEntry *entries,
                            std::size_t entry_count, const char *count, const char *count_display,
                            int damage );
void fluent_catalog_free( void *catalog );
void fluent_string_free( char *value );
}

using Forms = std::map<std::string, std::string>;

struct Entity {
    Forms en;
    Forms ru;
    std::string ru_gender = "";
};

struct Scenario {
    std::string label;
    std::string event;
    std::map<std::string, Entity> roles;
    std::optional<std::string> count;
    int damage = -1;
    std::string expected;
    bool expect_fallback = false;
};

struct Rendered {
    std::string text;
    bool fallback;
};

std::string read_file( const std::string &path )
{
    std::ifstream input( path );
    if( !input ) {
        throw std::runtime_error( "Cannot read " + path );
    }
    return { std::istreambuf_iterator<char>( input ), std::istreambuf_iterator<char>() };
}

struct CountInfo {
    std::string display;
    std::string category;
};

CountInfo format_count( const std::string &count, const char *locale )
{
    UErrorCode status = U_ZERO_ERROR;
    const std::size_t decimal = count.find( '.' );
    const int32_t digits = decimal == std::string::npos ? 0 :
                           static_cast<int32_t>( count.size() - decimal - 1 );
    const icu::Locale language( locale );
    auto formatter = icu::number::NumberFormatter::withLocale( language ).precision(
                         icu::number::Precision::fixedFraction( digits ) );
    auto formatted = formatter.formatDecimal( icu::StringPiece( count ), status );
    std::unique_ptr<icu::PluralRules> rules( icu::PluralRules::forLocale( language, status ) );
    if( U_FAILURE( status ) || !rules ) {
        throw std::runtime_error( "Cannot format number or load plural rules" );
    }
    icu::UnicodeString category = rules->select( formatted, status );
    icu::UnicodeString display = formatted.toString( status );
    if( U_FAILURE( status ) ) {
        throw std::runtime_error( "Cannot select plural category" );
    }
    CountInfo result;
    display.toUTF8String( result.display );
    category.toUTF8String( result.category );
    return result;
}

std::vector<Scenario> scenarios()
{
    const Entity giant_wasp { { { "base", "The giant wasp" }, { "sentence", "The giant wasp" } },
        { { "nom", "Гигантская оса" } }, "feminine" };
    const Entity fungal_target { { { "base", "the fungal wasp" } },
        { { "acc", "грибную осу" } } };
    const Entity fungal_attacker { { { "base", "The fungal wasp" } },
        { { "nom", "Грибная оса" } } };
    const Entity feral_attacker { { { "base", "The feral human" },
            { "possessive", "the feral human's" }, { "sentence", "The feral human" } },
        { { "nom", "Дикий человек" }, { "gen", "дикого человека" } }, "masculine" };
    const Entity feral_target { { { "base", "the feral human" } },
        { { "acc", "дикого человека" } } };
    const Entity torso { { { "base", "the torso" } }, { { "acc", "торс" } } };
    const Entity arm { { { "base", "the arm" } }, { { "acc", "руку" } } };
    const Entity maria { { { "base", "Maria" } }, { { "nom", "Мария" } } };
    const Entity grass { { { "base", "the grass" } }, { { "acc", "траву" } } };
    const Entity named_wasp { { { "base", "The giant wasp nicknamed Buzz" } },
        { { "nom", "Гигантская оса по прозвищу «Жужа»" } } };
    const Entity unknown_mod { { { "base", "the unknown crawler" },
            { "sentence", "The unknown crawler" } },
        { { "nom", "неизвестный ползун" } } };
    const Entity bent_pipe { { { "base", "the bent steel pipe" } },
        { { "inst", "погнутой стальной трубой" } } };

    std::vector<Scenario> result {
        { "wasp bite", "bite-hit", { { "attacker", giant_wasp }, { "target", fungal_target },
                { "body", torso } }, {}, -1, "Гигантская оса кусает грибную осу в торс!" },
        { "fungal wasp bite", "fungal-bite-hit", { { "attacker", fungal_attacker },
                { "target", feral_target }, { "body", arm } }, {}, -1,
          "Грибная оса кусает дикого человека за руку!" },
        { "NPC dodge", "dodge", { { "defender", maria }, { "attacker", feral_attacker } },
          {}, -1, "Мария уворачивается от атаки дикого человека." },
        { "two feral humans, forced target", "shoot", { { "attacker", feral_attacker },
                { "target", feral_target } }, {}, -1,
          "Дикий человек стреляет в дикого человека." },
        { "landing on grass", "land", { { "surface", grass } }, {}, -1,
          "Вы приземлились на траву." },
        { "slam damage", "slam-damage", { { "obstacle", grass } }, {}, 3,
          "Вас швырнуло на траву, вы получили 3 урона." },
        { "slam without damage", "slam-no-damage", { { "obstacle", grass } }, {}, -1,
          "Вас швырнуло на траву, но вы не пострадали." },
        { "rapid strike without damage", "rapid-no-damage", { { "target", feral_target } },
          {}, -1, "Вы стремительно ударили дикого человека, но не нанесли урона." },
        { "modified item name", "item-hit", { { "target", feral_target },
                { "item", bent_pipe } }, {}, -1,
          "Вы ударили дикого человека погнутой стальной трубой." },
        { "compound name with nickname", "bite-hit", { { "attacker", named_wasp },
                { "target", fungal_target }, { "body", torso } }, {}, -1,
          "Гигантская оса по прозвищу «Жужа» кусает грибную осу в торс!" },
        { "gender agreement feminine", "lunge", { { "attacker", giant_wasp },
                { "target", fungal_target } }, {}, -1,
          "Гигантская оса бросилась на грибную осу." },
        { "gender agreement masculine", "lunge", { { "attacker", feral_attacker },
                { "target", fungal_target } }, {}, -1,
          "Дикий человек бросился на грибную осу." },
        { "unknown mod gender", "lunge", { { "attacker", unknown_mod },
                { "target", fungal_target } }, {}, -1,
          "The unknown crawler lunges at the fungal wasp.", true },
        { "unknown mod name", "rapid-no-damage", { { "target", unknown_mod } }, {}, -1,
          "You quickly strike the unknown crawler but deal no damage.", true },
        { "missing Russian event", "source-only", { { "target", grass } }, {}, -1,
          "You inspect the grass.", true },
    };
    for( const auto &[count, expected] : std::vector<std::pair<std::string, std::string>> {
            { "0", "Вы собрали 0 килограммов травы." },
            { "1", "Вы собрали 1 килограмм травы." },
            { "2", "Вы собрали 2 килограмма травы." },
            { "5", "Вы собрали 5 килограммов травы." },
            { "11", "Вы собрали 11 килограммов травы." },
            { "21", "Вы собрали 21 килограмм травы." },
            { "22", "Вы собрали 22 килограмма травы." },
            { "1.5", "Вы собрали 1,5 килограмма травы." },
            { "2.0", "Вы собрали 2,0 килограмма травы." }
        } ) {
        result.push_back( { "grass weight " + count, "grass-weight", {}, count, -1, expected } );
    }
    return result;
}

class IcuCatalog
{
    struct Entry {
        std::unique_ptr<icu::MessageFormat> formatter;
        std::vector<std::string> arguments;
    };
    std::map<std::string, Entry> entries;

    public:
        IcuCatalog( const std::string &locale, const std::string &path ) {
            std::ifstream input( path );
            if( !input ) {
                throw std::runtime_error( "Cannot read " + path );
            }
            std::string line;
            while( std::getline( input, line ) ) {
                const std::size_t separator = line.find( '\t' );
                if( separator == std::string::npos ) {
                    throw std::runtime_error( "Invalid ICU catalog line" );
                }
                const std::string id = line.substr( 0, separator );
                const icu::UnicodeString pattern = icu::UnicodeString::fromUTF8(
                        line.substr( separator + 1 ) );
                UErrorCode status = U_ZERO_ERROR;
                icu::MessagePattern parsed( status );
                parsed.parse( pattern, nullptr, status );
                if( U_FAILURE( status ) ) {
                    throw std::runtime_error( "Invalid ICU pattern: " + id );
                }
                Entry entry;
                for( int32_t part = 0; part < parsed.countParts(); ++part ) {
                    if( parsed.getPartType( part ) == UMSGPAT_PART_TYPE_ARG_NAME ) {
                        std::string name;
                        parsed.getSubstring( parsed.getPart( part ) ).toUTF8String( name );
                        if( std::find( entry.arguments.begin(), entry.arguments.end(), name ) ==
                            entry.arguments.end() ) {
                            entry.arguments.push_back( name );
                        }
                    }
                }
                entry.formatter = std::make_unique<icu::MessageFormat>( pattern,
                                  icu::Locale( locale.c_str() ), status );
                if( U_FAILURE( status ) ) {
                    throw std::runtime_error( "Cannot compile ICU pattern: " + id );
                }
                entries.emplace( id, std::move( entry ) );
            }
        }

        std::optional<std::string> render( const Scenario &scenario, bool english ) const {
            const auto found = entries.find( scenario.event );
            if( found == entries.end() ) {
                return std::nullopt;
            }
            std::vector<icu::UnicodeString> names;
            std::vector<icu::Formattable> values;
            const std::optional<CountInfo> count_info = scenario.count ?
                    std::make_optional( format_count( *scenario.count, english ? "en" : "ru" ) ) :
                    std::nullopt;
            for( const std::string &argument : found->second.arguments ) {
                names.push_back( icu::UnicodeString::fromUTF8( argument ) );
                if( argument == "count_category" && count_info ) {
                    values.emplace_back( icu::UnicodeString::fromUTF8( count_info->category ) );
                } else if( argument == "count_display" && count_info ) {
                    values.emplace_back( icu::UnicodeString::fromUTF8( count_info->display ) );
                } else if( argument == "damage" && scenario.damage >= 0 ) {
                    values.emplace_back( scenario.damage );
                } else {
                    const std::size_t split = argument.rfind( '_' );
                    if( split == std::string::npos ) {
                        return std::nullopt;
                    }
                    const auto role = scenario.roles.find( argument.substr( 0, split ) );
                    if( role == scenario.roles.end() ) {
                        return std::nullopt;
                    }
                    if( argument.substr( split + 1 ) == "gender" ) {
                        if( english || role->second.ru_gender.empty() ) {
                            return std::nullopt;
                        }
                        values.emplace_back( icu::UnicodeString::fromUTF8( role->second.ru_gender ) );
                        continue;
                    }
                    const Forms &forms = english ? role->second.en : role->second.ru;
                    const auto form = forms.find( argument.substr( split + 1 ) );
                    if( form == forms.end() ) {
                        return std::nullopt;
                    }
                    values.emplace_back( icu::UnicodeString::fromUTF8( form->second ) );
                }
            }
            UErrorCode status = U_ZERO_ERROR;
            icu::UnicodeString output;
            found->second.formatter->format( names.data(), values.data(),
                                             static_cast<int32_t>( names.size() ), output, status );
            if( U_FAILURE( status ) ) {
                return std::nullopt;
            }
            std::string result;
            output.toUTF8String( result );
            return result;
        }
};

class FluentCatalog
{
    void *en;
    void *ru;

    static std::optional<std::string> render_one( void *catalog, const Scenario &scenario,
            bool english ) {
        std::vector<FormEntry> entries;
        for( const auto &[role, entity] : scenario.roles ) {
            const Forms &forms = english ? entity.en : entity.ru;
            if( !english && !entity.ru_gender.empty() ) {
                entries.push_back( { role.c_str(), "gender", entity.ru_gender.c_str() } );
            }
            for( const auto &[form, value] : forms ) {
                entries.push_back( { role.c_str(), form.c_str(), value.c_str() } );
            }
        }
        const std::optional<CountInfo> count_info = scenario.count ?
                std::make_optional( format_count( *scenario.count, english ? "en" : "ru" ) ) :
                std::nullopt;
        char *text = fluent_catalog_render( catalog, scenario.event.c_str(), entries.data(),
                                            entries.size(), scenario.count ? scenario.count->c_str() : nullptr,
                                            count_info ? count_info->display.c_str() : nullptr,
                                            scenario.damage );
        if( !text ) {
            return std::nullopt;
        }
        std::string result( text );
        fluent_string_free( text );
        return result;
    }

    public:
        explicit FluentCatalog( const std::string &directory ) {
            const std::string en_text = read_file( directory + "/en.ftl" );
            const std::string ru_text = read_file( directory + "/ru.ftl" );
            en = fluent_catalog_new( "en", en_text.c_str() );
            ru = fluent_catalog_new( "ru", ru_text.c_str() );
            if( !en || !ru ) {
                fluent_catalog_free( en );
                fluent_catalog_free( ru );
                throw std::runtime_error( "Cannot compile Fluent catalogs" );
            }
        }
        ~FluentCatalog() {
            fluent_catalog_free( en );
            fluent_catalog_free( ru );
        }
        std::string render_english( const Scenario &scenario ) const {
            auto english = render_one( en, scenario, true );
            if( !english ) {
                throw std::runtime_error( "Missing English Fluent event: " + scenario.event );
            }
            return *english;
        }
        Rendered render( const Scenario &scenario ) const {
            if( auto translated = render_one( ru, scenario, false ) ) {
                return { *translated, false };
            }
            return { render_english( scenario ), true };
        }
};

int main( int argc, char **argv )
{
    if( argc != 2 ) {
        std::cerr << "Usage: i18n-spike <resource-directory>\n";
        return 2;
    }
    try {
        const std::string directory = argv[1];
        IcuCatalog icu_en( "en", directory + "/en.icu" );
        IcuCatalog icu_ru( "ru", directory + "/ru.icu" );
        FluentCatalog fluent( directory );
        const std::vector<Scenario> cases = scenarios();
        int failures = 0;
        for( const Scenario &scenario : cases ) {
            std::optional<std::string> icu_text = icu_ru.render( scenario, false );
            const bool icu_fallback = !icu_text;
            if( !icu_text ) {
                icu_text = icu_en.render( scenario, true );
            }
            if( !icu_text ) {
                throw std::runtime_error( "Missing English ICU event: " + scenario.event );
            }
            const Rendered fluent_text = fluent.render( scenario );
            const std::optional<std::string> english_icu = icu_en.render( scenario, true );
            const std::string english_fluent = fluent.render_english( scenario );
            const bool valid = *icu_text == scenario.expected &&
                               fluent_text.text == scenario.expected &&
                               icu_fallback == scenario.expect_fallback &&
                               fluent_text.fallback == scenario.expect_fallback &&
                               english_icu && *english_icu == english_fluent;
            std::cout << ( valid ? "OK " : "FAIL " ) << scenario.label << '\n'
                      << "  ICU:    " << *icu_text << ( icu_fallback ? " [en fallback]" : "" ) << '\n'
                      << "  Fluent: " << fluent_text.text <<
                      ( fluent_text.fallback ? " [en fallback]" : "" ) << '\n';
            failures += !valid;
        }
        for( const auto &[count, category] : std::vector<std::pair<std::string, std::string>> {
                { "0", "many" }, { "1", "one" }, { "2", "few" }, { "5", "many" },
                { "11", "many" }, { "21", "one" }, { "22", "few" },
                { "1.5", "other" }, { "2.0", "other" }
            } ) {
            const Scenario category_case { "plural category " + count, "plural-category", {},
                                           count, -1, category };
            const auto icu_category = icu_ru.render( category_case, false );
            const Rendered fluent_category = fluent.render( category_case );
            const bool valid = icu_category && *icu_category == category &&
                               fluent_category.text == category && !fluent_category.fallback;
            std::cout << ( valid ? "OK " : "FAIL " ) << "plural category " << count
                      << ": ICU=" << ( icu_category ? *icu_category : "missing" )
                      << ", Fluent=" << fluent_category.text << '\n';
            failures += !valid;
        }
        const Scenario &hot = cases.front();
        const std::string before_icu = *icu_ru.render( hot, false );
        const std::string before_fluent = fluent.render( hot ).text;
        const bool switch_valid = icu_en.render( hot, true ) &&
                                  !fluent.render_english( hot ).empty() &&
                                  before_icu == *icu_ru.render( hot, false ) &&
                                  before_fluent == fluent.render( hot ).text;
        std::cout << ( switch_valid ? "OK" : "FAIL" ) << " ru -> en -> ru render sequence\n";
        failures += !switch_valid;
        for( const auto &[label, render] : std::vector<std::pair<std::string,
                 std::function<std::string()>>> {
                { "ICU", [&]() { return *icu_ru.render( hot, false ); } },
                { "Fluent", [&]() { return fluent.render( hot ).text; } }
            } ) {
            std::size_t bytes = 0;
            constexpr int iterations = 20000;
            const auto started = std::chrono::steady_clock::now();
            for( int i = 0; i < iterations; ++i ) {
                bytes += render().size();
            }
            const auto elapsed = std::chrono::steady_clock::now() - started;
            std::cout << std::fixed << std::setprecision( 2 ) << label << " warm: "
                      << std::chrono::duration<double, std::micro>( elapsed ).count() / iterations
                      << " us/call (" << bytes << " bytes rendered)\n";
        }
        return failures == 0 ? 0 : 1;
    } catch( const std::exception &error ) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
