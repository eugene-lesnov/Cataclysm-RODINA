#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "game.h"
#include "item.h"
#include "localized_event.h"
#include "mapdata.h"
#include "monster.h"
#include "npc.h"
#include "recipe.h"
#include "string_formatter.h"
#include "translation_manager.h"
#include "translations.h"
#include "type_id.h"

#if defined(LOCALIZE)

TEST_CASE( "localized_bite_events_use_real_creature_and_body_names", "[localized-event][translations]" )
{
    const std::string previous_language = TranslationManager::GetInstance().GetCurrentLanguage();
    on_out_of_scope restore_language( [&]() {
        set_language( previous_language );
    } );
    set_language( "ru" );

    monster wasp( mtype_id( "mon_wasp_small" ) );
    monster fungal_wasp( mtype_id( "mon_wasp_small_fungus" ) );
    monster wasp_guard( mtype_id( "mon_wasp_small_guard" ) );
    const bodypart_id arm( "arm_l" );
    const bodypart_id torso( "torso" );

    CHECK( format_bite_event( wasp, get_avatar(), arm, bite_event_result::miss ) ==
           "Оса-переросток бросается на вас, но вы уворачиваетесь!" );
    CHECK( format_bite_event( fungal_wasp, get_avatar(), arm, bite_event_result::no_damage ) ==
           "Грибная оса кусает вас в левую руку, но не может прокусить броню!" );
    CHECK( format_bite_event( wasp, fungal_wasp, torso, bite_event_result::damage ) ==
           "Оса-переросток кусает грибную осу в торс!" );
    CHECK( format_bite_event( fungal_wasp, wasp, torso, bite_event_result::no_damage ) ==
           "Грибная оса кусает осу-переростка в торс, но не может прокусить броню!" );
    CHECK( format_bite_event( fungal_wasp, wasp, torso, bite_event_result::miss ) ==
           "Грибная оса бросается на осу-переростка, но промахивается!" );
    CHECK( format_bite_event( wasp_guard, get_avatar(), arm, bite_event_result::damage ) ==
           "Переросшая оса-страж кусает вас в левую руку!" );
    CHECK( format_bite_event( wasp, wasp_guard, torso, bite_event_result::damage ) ==
           "The oversized wasp bites the oversized wasp guard in the torso!" );

    monster zombie( mtype_id( "mon_zombie" ) );
    CHECK( format_bite_event( wasp, zombie, torso, bite_event_result::damage ) ==
           "The oversized wasp bites the zombie in the torso!" );
    fungal_wasp.nickname = "Buzz";
    CHECK( format_bite_event( wasp, fungal_wasp, torso, bite_event_result::damage ) ==
           "The oversized wasp bites Buzz in the torso!" );

    set_language( "en" );
    CHECK( format_bite_event( wasp, get_avatar(), arm, bite_event_result::damage ) ==
           "The oversized wasp bites your left arm!" );
    set_language( "ru" );
    CHECK( format_bite_event( wasp, get_avatar(), arm, bite_event_result::damage ) ==
           "Оса-переросток кусает вас в левую руку!" );
}

TEST_CASE( "localized_impact_events_choose_surface_and_number_forms", "[localized-event][translations]" )
{
    const std::string previous_language = TranslationManager::GetInstance().GetCurrentLanguage();
    on_out_of_scope restore_language( [&]() {
        set_language( previous_language );
    } );
    set_language( "ru" );

    const ter_str_id grass_id( "t_grass" );
    const ter_str_id wall_id( "t_wall" );
    const ter_str_id white_grass_id( "t_grass_white" );
    const ter_t &grass = grass_id.obj();
    const ter_t &wall = wall_id.obj();
    const ter_t &white_grass = white_grass_id.obj();

    CHECK( format_impact_event( grass, impact_event_result::landing ) ==
           "Вы приземлились на траву." );
    CHECK( format_impact_event( grass, impact_event_result::slam_no_damage ) ==
           "Вас швырнуло на траву, но вы не пострадали." );
    CHECK( format_impact_event( wall, impact_event_result::slam_no_damage ) ==
           "Вы ударились о стену, но не пострадали." );
    CHECK( format_impact_event( wall, impact_event_result::slam_shock_absorbed ) ==
           "Вы ударились о стену, но амортизаторы погасили удар!" );
    CHECK( format_impact_event( grass, impact_event_result::slam_shock_absorbed ) ==
           "Вас швырнуло на траву, но амортизаторы погасили удар!" );
    CHECK( format_impact_event( grass, impact_event_result::landing_shock_absorbed ) ==
           "Вы приземлились на траву, но амортизаторы погасили удар!" );
    CHECK( format_impact_event( wall, impact_event_result::slam_damage, 5 ) ==
           "Вы ударились о стену и получили 5 единиц урона." );
    CHECK( format_impact_event( grass, impact_event_result::slam_damage, 2 ) ==
           "Вас швырнуло на траву, и вы получили 2 единицы урона." );

    for( const auto &[damage, expected] : std::vector<std::pair<int, std::string>> {
            { 0, "Вы приземлились на траву и получили 0 единиц урона." },
            { 1, "Вы приземлились на траву и получили 1 единицу урона." },
            { 2, "Вы приземлились на траву и получили 2 единицы урона." },
            { 5, "Вы приземлились на траву и получили 5 единиц урона." },
            { 11, "Вы приземлились на траву и получили 11 единиц урона." },
            { 21, "Вы приземлились на траву и получили 21 единицу урона." },
            { 22, "Вы приземлились на траву и получили 22 единицы урона." }
        } ) {
        CHECK( format_impact_event( grass, impact_event_result::landing_damage, damage ) == expected );
    }

    CHECK( format_impact_event( white_grass, impact_event_result::landing ) ==
           "You land on the white grass." );

    set_language( "fr" );
    CHECK( format_impact_event( wall, impact_event_result::slam_damage, 5 ) ==
           "You are slammed against the wall for 5 damage." );
}

TEST_CASE( "localized_combat_events_use_real_creature_forms", "[localized-event][translations]" )
{
    const std::string previous_language = TranslationManager::GetInstance().GetCurrentLanguage();
    on_out_of_scope restore_language( [&]() {
        set_language( previous_language );
    } );
    set_language( "ru" );

    monster feral_shooter( mtype_id( "mon_feral_human_pipe" ) );
    monster feral_target( mtype_id( "mon_feral_human_crowbar" ) );
    monster wasp( mtype_id( "mon_wasp_small" ) );
    monster zombie( mtype_id( "mon_zombie" ) );

    CHECK( format_shot_event( feral_shooter, feral_target ) ==
           "Дикий человек стреляет в дикого человека." );
    CHECK( format_dodge_event( get_avatar(), wasp ) ==
           "Вы уклоняетесь от атаки осы-переростка." );
    npc defender;
    defender.name = "Алексей";
    CHECK( format_dodge_event( defender, wasp ) ==
           "Алексей уклоняется от атаки осы-переростка." );
    CHECK( format_rapid_no_damage_event( feral_target ) ==
           "Вы быстро ударили дикого человека, но не нанесли урона." );
    CHECK_FALSE( format_shot_event( feral_shooter, zombie ) );
    CHECK_FALSE( format_rapid_no_damage_event( zombie ) );

    feral_target.nickname = "Bandit";
    CHECK( format_shot_event( feral_shooter, feral_target ) ==
           "The feral human shoots Bandit." );

    set_language( "en" );
    CHECK( format_shot_event( feral_shooter, feral_target ) ==
           "The feral human shoots Bandit." );
    set_language( "ru" );
    CHECK( format_dodge_event( get_avatar(), feral_shooter ) ==
           "Вы уклоняетесь от атаки дикого человека." );
}

TEST_CASE( "localized_item_events_keep_composite_labels_and_full_fallback",
           "[localized-event][translations]" )
{
    const std::string previous_language = TranslationManager::GetInstance().GetCurrentLanguage();
    on_out_of_scope restore_language( [&]() {
        set_language( previous_language );
    } );
    set_language( "ru" );

    item mod( itype_id( "battery_ups" ), calendar::turn );
    item tool( itype_id( "flashlight" ), calendar::turn );
    tool.set_var( "item_note", "prototype" );
    const recipe_id product_id( "spear_wood" );
    REQUIRE( product_id.is_valid() );
    const recipe &product = product_id.obj();

    CHECK( format_toolmod_event( mod, tool ) ==
           "На инструмент «" + tool.tname() + "» установлена модификация «" + mod.tname() + "»." );
    CHECK( format_craft_event( product, true ) ==
           "Изготовлено по памяти: " + product.result_name() + "." );
    CHECK( format_craft_event( product, false ) ==
           "Изготовлено по рецепту: " + product.result_name() + "." );

    set_language( "fr" );
    CHECK( format_toolmod_event( mod, tool ) == "You attach a modification to a tool." );
    CHECK( format_craft_event( product, true ) == "An item was crafted from memory." );

    set_language( "en" );
    CHECK( format_toolmod_event( mod, tool ) ==
           "You successfully attach " + mod.tname() + " to " + tool.tname() + "." );
}

TEST_CASE( "localized_quantity_keeps_visible_fraction_and_plural_category",
           "[localized-event][translations]" )
{
    const std::string previous_language = TranslationManager::GetInstance().GetCurrentLanguage();
    on_out_of_scope restore_language( [&]() {
        set_language( previous_language );
    } );
    set_language( "ru" );

    for( const auto &[decimal, display, category] :
         std::vector<std::tuple<std::string, std::string, std::string>> {
             { "0", "0", "many" }, { "1", "1", "one" }, { "2", "2", "few" },
             { "5", "5", "many" }, { "11", "11", "many" }, { "21", "21", "one" },
             { "22", "22", "few" }, { "1.5", "1,5", "other" },
             { "2.0", "2,0", "other" }
         } ) {
        const std::optional<localized_quantity> quantity = format_localized_quantity( decimal, "ru" );
        REQUIRE( quantity );
        CHECK( quantity->display == display );
        CHECK( quantity->category == category );
    }
    CHECK_FALSE( format_localized_quantity( "not a number", "ru" ) );
    CHECK( format_burning_duration_event( 1, true ) ==
           "Горение завершится примерно через 1 час." );
    CHECK( format_burning_duration_event( 2, true ) ==
           "Горение завершится примерно через 2 часа." );
    CHECK( format_burning_duration_event( 5, true ) ==
           "Горение завершится примерно через 5 часов." );
    CHECK( format_burning_duration_event( 21, false ) ==
           "Горение завершится примерно через 21 минуту." );
    CHECK( format_burning_duration_event( "1.5", true ) ==
           "Горение завершится примерно через 1,5 часа." );
    CHECK( format_burning_duration_event( "2.0", false ) ==
           "Горение завершится примерно через 2,0 минуты." );
    CHECK_FALSE( format_burning_duration_event( "invalid", true ) );
}

TEST_CASE( "localized_event_formatting_benchmark", "[.][benchmark][localized-event]" )
{
    const std::string previous_language = TranslationManager::GetInstance().GetCurrentLanguage();
    on_out_of_scope restore_language( [&]() {
        set_language( previous_language );
    } );
    monster wasp( mtype_id( "mon_wasp_small" ) );
    const bodypart_id arm( "arm_l" );

    set_language( "ru" );
    BENCHMARK( "Russian bite event" ) {
        return format_bite_event( wasp, get_avatar(), arm, bite_event_result::damage );
    };
    BENCHMARK( "legacy positional formatting" ) {
        return string_format( "%1$s кусает вас в %2$s!", wasp.name(),
                              body_part_name_accusative( arm ) );
    };
    BENCHMARK( "Russian duration event" ) {
        return format_burning_duration_event( 22, false );
    };

    set_language( "en" );
    BENCHMARK( "English bite event" ) {
        return format_bite_event( wasp, get_avatar(), arm, bite_event_result::damage );
    };
}

#endif
