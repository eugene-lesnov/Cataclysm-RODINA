bite-hit = { FORM($attacker, form: "nom") } кусает { FORM($target, form: "acc") } в { FORM($body, form: "acc") }!
fungal-bite-hit = { FORM($attacker, form: "nom") } кусает { FORM($target, form: "acc") } за { FORM($body, form: "acc") }!
dodge = { FORM($defender, form: "nom") } уворачивается от атаки { FORM($attacker, form: "gen") }.
shoot = { FORM($attacker, form: "nom") } стреляет в { FORM($target, form: "acc") }.
land = Вы приземлились на { FORM($surface, form: "acc") }.
slam-damage = Вас швырнуло на { FORM($obstacle, form: "acc") }, вы получили { $damage } урона.
slam-no-damage = Вас швырнуло на { FORM($obstacle, form: "acc") }, но вы не пострадали.
rapid-no-damage = Вы стремительно ударили { FORM($target, form: "acc") }, но не нанесли урона.
item-hit = Вы ударили { FORM($target, form: "acc") } { FORM($item, form: "inst") }.
lunge =
    { $attacker_gender ->
        [feminine] { FORM($attacker, form: "nom") } бросилась на { FORM($target, form: "acc") }.
        [masculine] { FORM($attacker, form: "nom") } бросился на { FORM($target, form: "acc") }.
       *[other] { FORM($attacker, form: "nom") } бросилось на { FORM($target, form: "acc") }.
    }
plural-category =
    { NUMBER($count) ->
        [one] one
        [few] few
        [many] many
       *[other] other
    }
grass-weight =
    { NUMBER($count) ->
        [one] Вы собрали { $count_display } килограмм травы.
        [few] Вы собрали { $count_display } килограмма травы.
        [many] Вы собрали { $count_display } килограммов травы.
       *[other] Вы собрали { $count_display } килограмма травы.
    }
