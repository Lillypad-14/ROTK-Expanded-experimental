# Ringwraith gear (all items, affixes, drops)

Source of truth: `hacks/patches/ringwraith/wraith_gear.c` (tables, rolls, names, info text), `wraith_gear.h` (effect and unique enums), `wraith_gear.asm` (hooks), `wraith_gear_tables.asm` (stock row copies), `ringwraith_kit.c` (where effects act).
Line numbers below are as of v8. Recheck them before trusting them.
How-to recipes for adding rows are in `howto-heroes-skills.md` section 5b. This doc is the reference of what exists.

## How it hooks into stock loot

| Piece | Where | What |
|---|---|---|
| `loot_generateItem` | `hookReplaceR3` in wraith_gear.asm:3 -> `wraithGear_generateItem` (wraith_gear.c:1052) | Ringwraith (`HERO_ID_RINGWRAITH`) calls `rollWraithItem`. Other heroes run a C copy of the stock function. |
| `item_affix_formatName` | wraith_gear.asm:4 -> `wraithGear_formatName` | Text ids 0x4000+ (`WRAITH_TEXT_ID_BASE`, wraith_gear.c:13) via `wraithGear_findString`. Uniques orange (palette index 6, wraith_gear.c:19..22), red fallback. |
| `item_getEquipSlot` | wraith_gear.asm:5 -> `wraithGear_getEquipSlot` | Other heroes cannot equip wraith gear. |
| ItemSuffixFlags literals | wraith_gear.asm:8..13 | 0x08026c40, 0x08026f4c, 0x080383dc repointed to `WraithItemSuffixFlags`. |
| Info panel | `wraithGear_describe` (wraith_gear.c:1203) | Stat lines, then effect lines, then the unique text. |
| Effects in play | `wraithGear_refresh` from `ringwraith_kitTick` (ringwraith_kit.c:876) | Sums equipped items into `WraithGearStats`. Kit reads `gear(p, WRAITH_FX_X)` and `hasUnique(p, WRAITH_UNIQUE_X)` (ringwraith_kit.c:323, 328). |

All hooks are inside `.if HACK_skill_expansion`.
The stock drop path is unchanged up to `loot_generateItem`: enemy death -> `loot_rollDrop` (drop chance from `EnemyLootInfos`) -> `loot_generateDrop` (gems/health/items quality roll) -> `loot_generateItem`.
So the chance that a kill or chest drops an item at all is the stock chance. The wraith only changes what the item is.
Chests and enemies roll the same wraith table (`unitType` is ignored on the wraith path).
The wraith's `RingwraithLootHeroItemTypeRolls` row (ringwraith_hook.asm:81) is a copy of the base hero's row and is not read on the wraith path of `wraithGear_generateItem`.

The extended tables keep stock rows first (copied by `wraith_gear_tables.asm`), and `baseItemCount` stays stock, so stock heroes never roll wraith rows.

## Roll (rollWraithItem, wraith_gear.c:998)

1. Slot: `rng_rollRange(1, 100)` walks `WraithDropSlots` by weight (table below, weights sum to 100).
2. Unique chance: `3 + g_LootTier / 3` percent, 0 when the slot has `uniqueCount == 0` (Mail).
3. One second roll `r = rng_rollRange(1, 100)`, `bonus = clamp(extraTreasurePercent / 10, 0, 20)` (magic find).
   - `r <= uniqueChance`: one of the slot's `uniqueBases`, picked uniformly. Uniques never get affixes.
   - else tiered base: `rollTiered(countAllowed(...))` = max of two uniform picks among the allowed tiers (leans high). Allowed tiers: tier <= `g_LootTier`, at least the first.
   - prefix if `r <= uniqueChance + 45 + bonus`.
   - suffix if `r <= uniqueChance + 10 + bonus / 2`.
   - Both use the same `r`, so a suffix always comes with a prefix. With no magic find: about 35% prefix only, 10% prefix + suffix, rest plain.
4. Affix pick: `pickAllowed`, uniform over rows with tier <= `g_LootTier + 1`, the first 2 rows always allowed.
5. The item word is `type | RUNE_NONE << 4 | base << 8 | prefix << 16 | suffix << 24`. The return value is the base's `lootType` (ground sprite).

There is no once-per-character rule for wraith uniques (stock uniques have one). Duplicates can drop.

## Rarity tiers

| Rarity | How | Name color |
|---|---|---|
| Plain | tiered base, no affix | stock |
| Prefixed | tiered base + prefix | stock |
| Prefix + suffix | tiered base + prefix + suffix | stock |
| Unique | `uniqueBases` row, `WRAITH_UNIQUE_FLAGS` = NO_PREFIX + NO_SUFFIX (wraith_gear.c:191) | orange |

Tiered bases carry item tiers 1, 3, 5, 7, 9 (5 tiers, `WRAITH_TIER_COUNT` wraith_gear.c:14). `g_LootTier` caps which tiers can roll.

## Drop slots (WraithDropSlots, wraith_gear.c:757)

Base indices are relative to the type's first wraith base (stock rows come first).

| Slot | Weight | Type | Tiered bases | Uniques |
|---|---|---|---|---|
| Mace | 7 | Mace | 0..4 | 5 Mace of the Witch-king |
| Maul | 7 | Mace | 6..10 | 11 Banner of Angmar |
| Blade | 5 | Sword1H | 0..4 | 5 The Morgul-Knife |
| Fang | 5 | Sword1H | 6..10 | 11 The Black Dart |
| Crown | 7 | Armor | 0..4 | 10 Iron Crown of Angmar |
| Hauberk | 7 | Armor | 5..9 | 11 Barrow-wight Hauberk |
| Helm | 7 | Armor | 12..16 | 22 Crown of the Barrow-king, 23 Mask of Dol Guldur |
| Mail | 7 | Armor | 17..21 | none |
| Ring | 6 | Accessory | 0..4 | 5 The Ninth Ring, 11 Carn Dum Signet, 13 Ring of Khamul |
| Pendant | 6 | Accessory | 6..10 | 12 Horn of Minas Morgul, 14 Voice of Sauron |
| Shroud | 6 | Cloak | 0..4 | 5 Shroud of Unbeing |
| Wrap | 6 | Cloak | 6..10 | 11 Shroud of the Dead Marshes |
| Gauntlets | 6 | Gloves | 0..4 | 5 Gauntlets of the Black Captain |
| Grips | 6 | Gloves | 6..10 | 11 The Unlit Hand |
| Greaves | 6 | Shoes | 0..4 | 5 Greaves of the Morgul Vale |
| Treads | 6 | Shoes | 6..10 | 11 Fell Rider's Spurs |

Wraith base counts per type: Mace 12, Sword1H 12, Armor 24, Accessory 15, Cloak 12, Gloves 12, Shoes 12 (wraith_gear.c:208..214).

## Tiered bases (stats, then wraith effects)

Each family has 5 tiers. Names use one tier word per tier: Grave / Barrow / Morgul / Dread / Black for the first family of a type, Wight / Carn Dum / Angmar / Udun / Nazgul for the second.
Values are listed tier 1 / 3 / 5 / 7 / 9. Effects come from the `*Effects` arrays (wraith_gear.c:582..632).

| Family | Stat A | Stat B | Wraith effects |
|---|---|---|---|
| Mace | Slash 7/12/17/22/28 | Crit% 5/10/10/15/20 | Dread per Hit 2/3/4/5/6 |
| Maul | Slash 8/13/18/23/29 | Courage 1/2/3/4/5 | Spirit per Terror 1/2/2/3/4, Dread per Hit 1/2/3/3/4 |
| Blade | Slash 6/10/14/19/24 | Crit% 10/12/15/18/22 | Life Drain% 1/1/2/2/3 |
| Fang | Slash 5/9/13/17/22 | Dodge% 2/3/4/5/6 | Unseen Damage% 10/15/20/25/35 |
| Crown | Melee armor 1/2/3/4/5 | Crit armor 3/6/10/14/20 | Fear Reach 4/6/8/10/12 |
| Hauberk | Melee armor 3/6/9/13/17 | Missile armor 2/4/7/10/13 | Ward% 2/3/4/5/6 |
| Helm | Melee armor 2/3/4/5/6 | Max Spirit 3/5/8/11/15 | Thrall Damage 1/2/3/4/5 |
| Mail | Melee armor 2/5/8/11/14 | Max HP 5/8/12/16/20 | Thrall Time 30/45/60/90/120 frames, Ward% 1/1/2/2/3 |
| Ring | Courage 1/2/3/4/5 | Max Spirit -/-/5/10/15 | Spirit per Kill 1/1/2/2/3 |
| Pendant | Max Spirit 5/8/10/15/20 | Courage -/-/2/3/4 | Spirit per Terror 1/1/2/2/3, Fear Reach 3/4/6/8/10 |
| Shroud | Dodge% 2/4/6/8/10 | Missile armor 1/2/3/4/6 | Unseen 15/20/25/30/40 frames |
| Wrap | Dodge% 3/5/7/9/12 | Speed% 1/2/3/4/5 | Unseen 10/15/20/25/30 frames, Unseen Damage% 5/8/10/12/15 |
| Gauntlets | Strength 1/2/3/4/5 | Crit% 2/3/4/5/6 | Dmg vs Frightened% 4/6/8/10/12 |
| Grips | Accuracy 1/2/3/4/5 | Crit% 3/4/5/6/8 | Unseen Damage% 5/8/12/15/20, Life Drain% 1/1/1/2/2 |
| Greaves | Speed% 2/4/5/6/8 | Defense 1/1/2/2/3 | Drain Healing% 5/8/11/14/18 |
| Treads | Speed% 3/4/6/7/9 | Courage 1/1/2/2/3 | Terror 10/15/20/25/30 frames, Fear Reach 2/3/4/5/6 |

## Uniques (WRAITH_UNIQUE_*, wraith_gear.h:30..49, texts wraith_gear.c UniqueDescriptions)

| Id | Name | Slot | Item tier | Stats | Power |
|---|---|---|---|---|---|
| 0 WITCH_KINGS_MACE | Mace of the Witch-king | Mace | 6 | Slash 30, Crit% 20, Max Spirit -20 | Kills spread +40 Dread to nearby foes |
| 1 MORGUL_KNIFE | The Morgul-Knife | Blade | 6 | Slash 14, Crit% 25 | Melee hits embed Morgul shards, -25% melee damage |
| 2 IRON_CROWN | Iron Crown of Angmar | Crown | 7 | Melee armor 4, Courage 8, Fear resist% 50 | Double fear reach, Dread never fades, +15% damage taken |
| 3 BARROW_HAUBERK | Barrow-wight Hauberk | Hauberk | 7 | Melee armor 16, Missile armor 14, Max Spirit 20 | Damage drains Spirit first, 1 Spirit per 2 damage |
| 4 NINTH_RING | The Ninth Ring | Ring | 7 | Courage 6, Crit% 9 | Every ninth melee hit terrifies |
| 5 SHROUD_OF_UNBEING | Shroud of Unbeing | Shroud | 7 | Dodge% 12, Missile armor 6 | Hits keep Unseen, Unseen half as long, Unseen blows 5x |
| 6 BLACK_CAPTAIN | Gauntlets of the Black Captain | Gauntlets | 8 | Strength 6, Crit% 9 | Double damage and +10% Life Drain vs foes at full Dread |
| 7 MORGUL_VALE | Greaves of the Morgul Vale | Greaves | 7 | Speed% 10, Defense 4 | Every 2 s the nearest foe gains +25 Dread |
| 8 BANNER_OF_ANGMAR | Banner of Angmar | Maul | 7 | Slash 24, Max Spirit 15 | One more thrall, thralls +3 damage |
| 9 BARROW_CROWN | Crown of the Barrow-king | Helm | 7 | Melee armor 5, Max Spirit 20 | Foes near thralls gain +15 Dread each pulse |
| 10 CARN_DUM_SIGNET | Carn Dum Signet | Ring | 7 | Courage 5, Max Spirit 15 | Command of the Nine spares one thrall |
| 11 BLACK_DART | The Black Dart | Fang | 7 | Slash 18, Crit% 20 | Unseen blows plant a Morgul shard |
| 12 DEAD_MARSHES | Shroud of the Dead Marshes | Wrap | 7 | Dodge% 10, Max Spirit 15 | Unseen World lasts 50% longer |
| 13 UNLIT_HAND | The Unlit Hand | Grips | 7 | Accuracy 6, Crit% 10 | Shadowstep strike kill refunds its Spirit |
| 14 MORGUL_HORN | Horn of Minas Morgul | Pendant | 7 | Courage 6, Max Spirit 10 | Cry double damage, refunds half its Spirit |
| 15 DOL_GULDUR_MASK | Mask of Dol Guldur | Helm | 8 | Melee armor 6, Courage 6 | Terrified foes burst on death for 8 damage and +50 Dread |
| 16 VOICE_OF_SAURON | Voice of Sauron | Pendant | 8 | Courage 8, Max Spirit 20 | Aura half as wide, double aura Dread |
| 17 RING_OF_KHAMUL | Ring of Khamul | Ring | 8 | Courage 5, Max HP 20 | Every kill heals 3% health |
| 18 FELL_BEAST_SADDLE | Fell Rider's Spurs | Treads | 7 | Speed% 12, Courage 4 | Fell Beast twice as wide and terrifies |

The unique's "item tier" is its stat-record tier. It does not gate the unique roll (only the unique chance formula does).
Cap: `WraithGearStats.uniques` is a u32 mask, so at most 32 uniques.

## Effects (WRAITH_FX_*, wraith_gear.h:10..24)

| Id | Enum | Label | Unit | Used in ringwraith_kit.c |
|---|---|---|---|---|
| 1 | DREAD_PER_HIT | Dread per Hit | Dread | melee on-hit Dread |
| 2 | FEAR_REACH | Fear Reach | px radius | aura radius (:685) |
| 3 | LIFE_DRAIN | % Life Drain | % of melee damage | `drainPercent` (:965) |
| 4 | KILL_SPIRIT | Spirit per Kill | Spirit | on kill |
| 5 | FRIGHT_DAMAGE | % Dmg vs Frightened | % | melee damage (:962) |
| 6 | TERROR_STUN | s Terror | frames, shown as seconds | terror stun length (:488) |
| 7 | UNSEEN | s Unseen | frames, shown as seconds | Unseen World length |
| 8 | DRAIN_HEAL | % Drain Healing | % | Black Breath / No Living Man heal (:800) |
| 9 | WARD | % Damage Ward | % less damage | damage taken |
| 10 | THRALL_DAMAGE | Thrall Damage | flat | thrall hits (:613) |
| 11 | THRALL_FRAMES | s Thrall Time | frames, shown as seconds | thrall lifetime (:607) |
| 12 | UNSEEN_DAMAGE | % Unseen Damage | % | Unseen blows |
| 13 | TERROR_SPIRIT | Spirit per Terror | Spirit | on terrify (:492) |

Frame values display as seconds (value / 60, one decimal) only for TERROR_STUN, UNSEEN and THRALL_FRAMES (`appendEffect`, wraith_gear.c:1183).
Each base, prefix or suffix row has up to 2 effects (`FX1` / `FX2`) or one unique (`UNIQUE`). Effects sum over all equipped items.

## Prefixes (WRAITH_PREFIX_RECORDS, wraith_gear.c:501, 20 rows)

The same 20 records are appended to all 5 stock prefix tables (Weapon, Armor, Accessory, Cloak, GloveShoe), so every wraith type can roll every prefix.
Rows 0 and 1 are always allowed. The rest need tier <= `g_LootTier + 1`.

| # | Name | Tier | Stat | Effect (PrefixEffects) |
|---|---|---|---|---|
| 0 | Dreadful | 1 | | Dread per Hit 4 |
| 1 | Cold | 1 | Melee armor 3 | Ward 3% |
| 2 | Hungering | 2 | | Life Drain 2% |
| 3 | Shadowed | 2 | Dodge 4% | Unseen 20 frames |
| 4 | Withering | 3 | Slash 4 | Dmg vs Frightened 8% |
| 5 | Grave-cold | 3 | Fear resist 10% | Ward 5% |
| 6 | Terrible | 4 | | Dread per Hit 8 |
| 7 | Commanding | 4 | | Thrall Damage 3 |
| 8 | Ravenous | 5 | | Life Drain 4% |
| 9 | Sorcerous | 5 | Max Spirit 10 | Spirit per Kill 2 |
| 10 | Petrifying | 6 | | Terror 30 frames |
| 11 | Deathless | 6 | Max HP 15 | Drain Healing 15% |
| 12 | Soul-eating | 7 | | Spirit per Kill 3, Life Drain 3% |
| 13 | Undying | 8 | Max HP 25 | Ward 8% |
| 14 | Abyssal | 8 | | Fear Reach 12, Dread per Hit 6 |
| 15 | Nameless | 9 | Slash 8 | Dmg vs Frightened 15% |
| 16 | Gravebound | 3 | | Thrall Time 60 frames |
| 17 | Lurking | 2 | | Unseen Damage 15% |
| 18 | Harrowing | 4 | | Spirit per Terror 3 |
| 19 | Necromantic | 6 | | Thrall Damage 3, Thrall Time 45 frames |

## Suffixes (WraithItemSuffixes, wraith_gear.c:543, 16 rows)

Appended after the stock `ItemSuffixes` (one shared table for all types).

| # | Name | Tier | Stat | Effect (SuffixEffects) |
|---|---|---|---|---|
| 0 | of the Nine | 1 | Crit 5% | Dread per Hit 3 |
| 1 | of the Barrows | 1 | Melee armor 4 | Ward 3% |
| 2 | of Shadow | 2 | Dodge 5% | Unseen 30 frames |
| 3 | of Cold Iron | 2 | Slash 3 | Terror 15 frames |
| 4 | of Hunger | 3 | | Life Drain 3% |
| 5 | of Thralldom | 3 | | Thrall Damage 4 |
| 6 | of Angmar | 4 | | Fear Reach 10 |
| 7 | of Despair | 4 | | Dmg vs Frightened 12% |
| 8 | of Dol Guldur | 5 | | Drain Healing 20% |
| 9 | of Minas Morgul | 5 | Max Spirit 15 | Spirit per Kill 2 |
| 10 | of the Witch-king | 6 | | Dread per Hit 6, Terror 20 frames |
| 11 | of the Dark Lord | 7 | Slash 6 | Fear Reach 8, Dmg vs Frightened 8% |
| 12 | of the Grave | 2 | | Thrall Time 45 frames |
| 13 | of the Unseen | 3 | | Unseen Damage 20% |
| 14 | of Terror | 5 | | Spirit per Terror 4 |
| 15 | of the Host | 4 | | Thrall Damage 3, Thrall Time 30 frames |

## Adding things

Follow `howto-heroes-skills.md` section 5b recipes (tiered family, unique, effect, prefix/suffix). Short version:
- Everything is append only. Saved items store base, prefix and suffix indices.
- A base needs a row in 4 parallel tables (`*Items`, `*Gfx`, `*ItemFlags`, `*Effects`) and the type's `*_COUNT` bump. The `_Static_assert` block catches a missing row.
- A new family needs a `WraithDropSlots` row. Take weight from other slots so the sum stays 100 (not checked by the build).
- A unique needs `WRAITH_UNIQUE_X` in wraith_gear.h, a `uniqueBases` entry (max 4 per slot, `WRAITH_SLOT_UNIQUE_MAX`), a `UniqueDescriptions` text and its code in ringwraith_kit.c behind `hasUnique`.
- A new effect needs `WRAITH_FX_X` before `WRAITH_FX_COUNT`, an `EffectLabels` entry, the `appendEffect` frames list if it is in frames, and a `gear(p, WRAITH_FX_X)` read in the kit.
- Prefix: append a record to `WRAITH_PREFIX_RECORDS` and a row to `PrefixEffects`, bump `WRAITH_PREFIX_COUNT`. Suffix: same with `WraithItemSuffixes`, `SuffixEffects`, `WRAITH_SUFFIX_COUNT`. Stock count plus wraith count must stay below 0xff.
- Tuning knobs: unique chance `3 + g_LootTier / 3` and prefix/suffix thresholds 45 / 10 are literals in `rollWraithItem`.

## Inconsistencies seen

- `WraithDropSlot.uniqueCount` comment says 1..WRAITH_SLOT_UNIQUE_MAX, but the Mail slot uses 0 (handled: unique chance becomes 0).
- `rollWraithItem` names the magic find input `extraTreasurePercent`. Prefix and suffix share one roll, so a suffix without a prefix cannot drop.
