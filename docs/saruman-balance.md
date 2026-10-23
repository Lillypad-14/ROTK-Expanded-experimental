# Saruman spell balance

This pass reduces spell damage and makes sustained casting consume Spirit.
Values are before enemy mitigation, empowerment and other passive bonuses.
Costs stay fixed as ranks increase.

| Spell | Spirit before | Spirit now | Rank 1 / rank 5 damage or effect |
| --- | ---: | ---: | --- |
| Voice of Saruman | 10 | 21 | 7 / 19 damage and area stun |
| Flame of Orthanc | 10 | 21 | 15 / 35 direct damage, then 2 / 6 burn damage per second for 5 / 9 seconds |
| Blasting Fire | 20 | 42 | 27 / 55 damage per target and short stun |
| Many-Coloured Ward | 20 | 35 | 25% / 45% damage reduction for 8 / 16 seconds |
| Word of Binding | 14 | 28 | 9 / 21 damage, 5 / 9 second hold, +25% spell damage against target |
| Orthanc's Remedy | 18 | 37 | 20 / 52 HP before Keeper |
| Echo of Power | 30 | 37 | Repeat last eligible spell at 50% / 90% power |
| Furnaces of Isengard | 50 | 63 | Up to five targets, 4 / 12 burn damage per second for six seconds |
| Sharkey's Curse | 25 | 35 | 8 / 20 damage and +25% / +45% spell vulnerability |
| Light Broken | 30 | 63 | 38 / 78 damage per target |
| Sharkey's Spite | 35 | 52 | 19 / 39 damage per target, scaling with missing HP |
| Power of the White Hand | 90 | 104 | 48 / 96 damage per target and area stun |

The three-cast empowerment mechanic and cooldowns remain in place.
Reserve grants one Spirit per second, down from two.
Damaging staff hits still grant two Spirit before Wisdom and Treachery bonuses.

Refunds are bounded or reduced:

- Keeper converts overhealing into at most eight Spirit per cast, including an Echo of Remedy.
- Curse grants one Spirit per damaging direct hit, with no return from burn ticks or zero-damage hits.
- Spite returns two Spirit per target, up to eight total.
- Ring-lore returns two Spirit per rank on empowerment consumption, and Council adds six.
- Machine of War grants one Spirit per rank on spell kills.
- Cutpurse grants three Spirit per rank on cursed kills.

At maximum Ring-lore with Council, four Flame casts cost 84 Spirit and return 16, a net cost of 68 before regeneration or kill rewards.
Previously that cycle cost 40 and returned 30, a net cost of ten.

`hacks/tests/saruman.c` covers costs, ranks, empowerment, capped refunds, periodic-hit exclusions and that four-cast resource cycle.
Normal gameplay still needs playtesting for encounter pacing and equipment-dependent regeneration.
