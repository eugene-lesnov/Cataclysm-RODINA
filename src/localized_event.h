#pragma once
#ifndef CATA_SRC_LOCALIZED_EVENT_H
#define CATA_SRC_LOCALIZED_EVENT_H

#include <optional>
#include <string>

#include "type_id.h"

class Creature;
class item;
class monster;
class recipe;
struct map_data_common_t;

enum class bite_event_result {
    miss,
    no_damage,
    damage
};

enum class impact_event_result {
    landing,
    landing_damage,
    landing_shock_absorbed,
    slam_no_damage,
    slam_shock_absorbed,
    slam_damage
};

std::optional<std::string> format_bite_event( const monster &attacker, const Creature &target,
        const bodypart_id &body, bite_event_result result );
std::optional<std::string> format_impact_event( const map_data_common_t &surface,
        impact_event_result result, int damage = 0 );
std::optional<std::string> format_shot_event( const Creature &attacker, const Creature &target );
std::optional<std::string> format_dodge_event( const Creature &defender, const monster &attacker );
std::optional<std::string> format_rapid_no_damage_event( const Creature &target );
std::optional<std::string> format_toolmod_event( const item &mod, const item &tool );
std::optional<std::string> format_craft_event( const recipe &product, bool from_memory );
std::optional<std::string> format_burning_duration_event( int count, bool hours );
std::optional<std::string> format_burning_duration_event( const std::string &decimal, bool hours );

struct localized_quantity {
    std::string display;
    std::string category;
};

std::optional<localized_quantity> format_localized_quantity( const std::string &decimal,
        const std::string &language );

#endif // CATA_SRC_LOCALIZED_EVENT_H
