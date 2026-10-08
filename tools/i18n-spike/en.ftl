bite-hit = { FORM($attacker, form: "base") } bites { FORM($target, form: "base") } in { FORM($body, form: "base") }!
fungal-bite-hit = { FORM($attacker, form: "base") } bites { FORM($target, form: "base") } in { FORM($body, form: "base") }!
dodge = { FORM($defender, form: "base") } dodges { FORM($attacker, form: "possessive") } attack.
shoot = { FORM($attacker, form: "base") } shoots { FORM($target, form: "base") }.
land = You land on { FORM($surface, form: "base") }.
slam-damage = You are slammed against { FORM($obstacle, form: "base") } for { $damage } damage.
slam-no-damage = You are slammed against { FORM($obstacle, form: "base") } but take no damage.
rapid-no-damage = You quickly strike { FORM($target, form: "base") } but deal no damage.
item-hit = You hit { FORM($target, form: "base") } with { FORM($item, form: "base") }.
lunge = { FORM($attacker, form: "sentence") } lunges at { FORM($target, form: "base") }.
source-only = You inspect { FORM($target, form: "base") }.
plural-category =
    { NUMBER($count) ->
        [one] one
       *[other] other
    }
grass-weight =
    { NUMBER($count) ->
        [one] You collected { $count_display } kilogram of grass.
       *[other] You collected { $count_display } kilograms of grass.
    }
