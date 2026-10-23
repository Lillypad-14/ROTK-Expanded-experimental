# How to: heroes, skills, items, loot

Step lists for the content work that keeps recurring. Low-level recipes (Ghidra, hooks, table
extension, build) are in `hacking-recipes.md`. Update this file whenever a step turns out wrong.

## Rules that bite (read before any change)

These fail silently or corrupt saves. The build catches only the ones marked (build).

1. **Append only, never insert.** Items save their base, prefix and suffix index. Inserting a row
   anywhere but the end of a wraith table changes every saved item after it. Uniques, effects and
   passive IDs are also append only.
2. **Parallel tables stay in step (build).** Each wraith base has a row in 4 tables: `*Items`,
   `*Gfx`, `*ItemFlags`, `*Effects`. Bump the `*_COUNT` define and add all 4 rows. The
   `_Static_assert` block in `wraith_gear.c` fails the build if a row count is off.
3. **Unique cap (build).** `WraithGearStats.uniques` is a u32 mask, so at most 32 wraith uniques.
4. **Page 2 actives order.** `SkillExpansionActiveDefs[hero][i]` lands on row 7 + i. Its index must
   match the hero's row enum (the Ringwraith's `RingwraithRow`), or the wrong callback runs.
5. **Drop weights sum to 100.** `WraithDropSlots` weights. Not checked by the build.
6. **Cooldown bit is shared.** A page 2 active uses its `baseRow`'s cooldown bit. Give each page 2
   active a different baseRow.
7. **New per-player state must be saved** if it should survive a reload (`skill_expansion_save.c`).
   Timers that reset on load are fine unsaved.
8. **No em-dashes in repo files.** Terse comments, one per struct field (AGENTS.md).
9. **Build, then copy to a new ROM name.** mGBA locks the open ROM.
10. **Every custom AoE, stun or Dread target goes through `skillExpansion_isFoe`** (skill_expansion_elite.c:160). It requires hp > 0, not NPC, not a player, not invulnerable, and `ACTOR_FLAG_COLLISION_CHECK`.
    Why: pillar/ledge climbers (enemy spawner variant 1/2, e.g. Moria) start in behaviorState 0x2f with collision cleared. The climb handler 0x0800b050 only advances while actionState == 2, so a hit mid-climb aborts it and strands the foe in the wall (fixed in v8). `actor_checkDeath` 0x0800a54c also clears collision when it sets dying state 0x11.
11. **Stock stun pattern** (copy it for new stuns): gate on hp > 0 and `actor_isStunnable`, then actionState 0x1a, `actionStateTimer = frames`, `field_0x64 |= 2`, `fx_attachStatusEffect(NULL, foe, frames, 0xc, 8, 0, 8)`.
12. **Modcode .bss is nearly full.** See hacking-recipes.md "Modcode RAM budget" before adding globals. Fetch hero state only after the isX guard (same section). Run the hero sweep (docs/test-arena.md) after skill work.

## Where things live

| What | File |
|---|---|
| Row/slot layout, accessors, def structs | `hacks/patches/skill_expansion/skill_expansion.h` |
| Page 2 actives + elite passives (data) | `skill_expansion/skill_expansion_content.c` |
| Elite skill callbacks + combat wrappers | `skill_expansion/skill_expansion_elite.c/.h` |
| Hero kit registry (`skillExpansion_getHeroKit`) | `skill_expansion/skill_expansion.c` |
| Skill tree page swap, tile draw, required levels | `skill_expansion/skill_expansion_tree.c` |
| HUD icon palettes | `skill_expansion/skill_expansion_hud.c` |
| Save of the new rows/slots | `skill_expansion/skill_expansion_save.c` |
| Ninth hero plumbing | `hacks/patches/ringwraith/ringwraith.c/.asm`, `ringwraith_hook.asm` |
| Ninth hero kit (worked example of a full custom kit) | `ringwraith/ringwraith_kit.c` |
| Hero-only gear (worked example) | `ringwraith/wraith_gear.c`, `wraith_gear.asm`, `wraith_gear_tables.asm` |
| Test cheats (max level, skill points) | Debug mod-options page, see memory `debug-cheats-patch` |

Layout: active rows 0..4 stock, 5 Herbal Healing, 6 empty, 7..12 page 2 (12 is optional). Passive slots 0..8 stock,
9..17 page 2. Always use the `skillExpansion_active*` / `skillExpansion_passive*` accessors, never
raw `PLAYER(p).activeSkillLevels[row]` for rows >= 6.

## 1. Add a page 2 active skill to a stock hero

1. In `skill_expansion_content.c`, write an `ActiveSkill` row: callback, spiritCost and cast fields
   copied from the stock row whose pose it borrows (see `KINGS_COMMAND_CAST_FIELDS`).
2. Write a `SkillExpansionActiveDef`: row, icon (any stock `ActiveSkill*Icon`, page 2 retints it),
   requiredLevels `{l1, l2, l3, l4, l5, -1}`, description with `@1` level and `@2` spirit, optional
   canCast, baseRow (the stock row it borrows its pose, cooldown bit and cast rules from).
3. Put it in `SkillExpansionActiveDefs[HERO_ID_X][0..4]`.
4. Callback: in `skill_expansion_elite.c` (declare it in `skill_expansion_elite.h`). Reuse helpers
   there: `stunFoe`, `skillExpansion_spawnBurstFx`, `raiseGhost`, `skillExpansion_isFoe`,
   `actor_iterateInRadius`, `findNextEnemy`. Level: `castLevel(playerIndex)`.
5. Ongoing effects (buff timers, on-hit, on-kill) go in `EliteState` plus `eliteTick` / `bonusDamage`
   / `onKill` in elite.c.
6. Alternative: set `stockCallback` to reuse a stock callback that reads baseRow's level.

## 2. Add a page 2 (elite) passive

1. Add an id to `enum SkillExpansionPassiveId` before `SKILL_EXPANSION_PASSIVE_END`.
2. Add its entry to `SkillExpansionElitePassives` with `ELITE_PASSIVE(maxLevel, firstLevel, ...)`,
   `iconPassiveSkillId` (stock icon) and description with `@1`. For custom art also set `.icon`
   (section 7). It wins over iconPassiveSkillId when non-NULL.
3. List it in `SkillExpansionPassiveIds[HERO_ID_X]` (9 per hero, `SKILL_EXPANSION_PASSIVE_ID_NONE`
   for empty).
   TRAP: the tree does NOT draw passives in list order. `skillExpansion_treeDrawPassiveGrid` sorts
   them by FIRST required level (ties keep list order) and fills the 3x3 grid row-major. For build
   columns (col 0/1/2 = the three builds) give each list entry a first level >= the previous one,
   e.g. Aragorn 10,11,12 / 13,14,15 / 17,20,21. Unequal levels shuffle skills across columns.

TEXT FIT: every description must wrap to at most 3 lines in the 208px box (font 0). Check with
`python -I hacks/tools/skill_text_fit.py hacks/patches/skill_expansion/*.c hacks/patches/ringwraith/*.c`
(exits 1 and prints the wrap of each overflow; it measures @1 as "Maxed", @2 as 3 digits).
Page 1 stock rows that a patch changes get new text in `AragornStockDescriptions` /
`FrodoStockDescriptions` (skill_expansion_content.c), read by `skillExpansion_getStockActiveDescription`.
4. Elite passives have no stat records. The effect code reads
   `skillExpansion_getPassiveLevel(playerIndex, ID)` wherever it applies (elite.c wrappers or the
   hero kit). Stock passives with stat records can also go on page 2 directly (see Frodo's list).

### Hero logic without a kit (Frodo pattern)
When page 1 stays stock, do not use a kit. Put data in skill_expansion_content.c and logic in
`skill_expansion_<hero>.c`, called from the existing wrappers in skill_expansion_elite.c (addHp, computePveDamage,
resolveMeleeHit/resolveRangedHit onHit, eliteTick, eliteReset/Cleanup, wrapStockCallbacks). Each entry point
returns early unless `PLAYER(p).heroId` matches. Charm: `actor_applyPersuasion(enemy, frames)` (0x080089ac),
charmed test `combatFlags.p & ACTOR_COMBAT_FLAG_PERSUADED`, timer `field_0x54`. Eligible: isFoe, ACTOR_TYPE_ENEMY,
variant != 0xd, no ACTOR_FLAG_NO_FLINCH. Watch baseRow cast rules in meetsHeroRequirements (Frodo Snare needs
Snare level > 0).

### Page 2 bought with kills (Gimli Tally pattern)
Gimli's page 2 is bought with Tally marks (1 per 10 kills) instead of skill points. To give another hero a non-point currency:
1. Make `skillExpansion_gimliUsesTally` (or a new `usesX(p)`) return TRUE for that hero. `tallyPage()` in skill_expansion_tree.c then routes A to `buyWithTally`, draws marks in the points box and "Tally: N kills" as the title.
2. requiredLevels of that page count in `GIMLI_TALLY_KILLS_PER_STEP` (20) kills, still <= 50 for the validator. Write "(Kills @1)" in descriptions, the tree multiplies @1 by the step.
3. Stock `scene_skillTree_buySkill` must never run on such a page, or it would also sell ranks for points (levels 1..20 pass its level check).
4. Spent marks = ranks bought on the page (activePurchased + passivePurchased of page 2). Old saves with ranks there count them as spent.
5. Effects keyed on kills read the killCount delta in the hero tick. See hacking-recipes.md "Kill counter".

## 3. Replace a hero's whole stock page (custom kit)

Follow `RingwraithHeroKit` in `ringwraith_kit.c`:
- `activeSkills[6]`, `icons[6]`, `requiredLevels[6][6]`, `descriptions[6]`, `canCast(p,row)`,
  `passiveSkillIds[9]`, `artHeroId` (whose tile art to use).
- Register it in `skillExpansion_getHeroKit`.
- Per-hero mechanics: a `<hero>_kit.c` with Reset/Tick/MeleeDamage/OnHit entry points, called from
  `skill_expansion_elite.c` under `#if HACK_<hero>` (see the `ringwraith_kit*` calls there).
- Wrap `player_addHp` / `player_addSpirit` only if the hero changes healing or damage taken (the
  Ringwraith does). Only one patch can own a whole-function hook, so extend the existing wrapper.

## 4. Add a new playable hero

The Ringwraith (HeroId 8) is the only template, and its plumbing hardcodes id 8 and a 9-entry
count. **Before a 10th hero, generalize it**: a hero registry (id, name, base hero, palette, banks,
kit) and table macros sized by `HERO_COUNT`, so each new hero is data, not another copy of
`ringwraith.asm`.

What a hero needs (all done for the Ringwraith, see `ringwraith-hero.md` for details):
1. **Tables extended past 8 heroes, relocated to modcode and every literal repointed**
   (`ringwraithRepoint` checks each site still holds the stock address):
   HeroMove/Melee/CastAnimationBanks, HeroPortraitBgAssets, SkillTreeHeroTileBgAssets,
   LootHeroRegionProgressions, LootHeroItemTypeRolls, HeroStartMissions, HeroStartVariants,
   spawn offsets, camera heights. Find xrefs with Ghidra, then confirm each site with the
   `readu32` check.
2. **Switches on heroId**: player_initItemsAndHollow and scene_backpack_enter (`cmp r0,#7` bounds and
   jump tables), the palette pick in actor_applyTypeDefaults, the skill tree title, and the item
   class alias sites (0x0802d674, 0x0802dbfe, 0x0802e270, 0x0802eab2).
3. **Stats and name**: `player_initHeroStats` wrap (HeroBaseStats), `ringwraith_getHeroName`.
4. **Sprites**: synthetic animation banks `[9 parts][7 poses]` built from existing clips, and skip
   hand sprites (`actor_player_updateHandSprites` wrap) if the body has no hand parts.
5. **Save/select screen**: `scene_saveLoadMenu_drawSlot` / `refreshSlot` wraps, unlock_all page.
6. **Skills**: a custom kit (section 3) plus `SkillExpansionHeroSkillBlocks[id]` and
   `SkillExpansionActiveDefs` / `SkillExpansionPassiveIds` rows for page 2.
7. `SKILL_EXPANSION_HERO_COUNT` up by one.

## 5. Items and loot

Source of truth: `include/item.h`, `include/loot.h`, `src/c/loot.c` (fully decompiled C),
`src/c/item_affix_formatName.c`. Read loot.c before changing drop logic. It is short and commented.

### Item handle (u32, `Item`)
`type | rune<<4 | base<<8 | prefix<<16 | suffix<<24`. 0xff = no prefix/suffix, `RUNE_NONE` = 0xf
no rune, type `ITEM_TYPE_EMPTY` 0xf = empty slot. Gem stacks: `(word & 0xff) == 0xff`.
Item types: KNIFE 0, SWORD_1H 1, SWORD_2H 2, AXE 3, MACE 4, STAFF 5, BOW 6, ARROW 7, ARMOR 8 (body
and helmets), SHIELD 9, ACCESSORY 10, CLOAK 11, GLOVES 12, SHOES 13, UNIQUE 14.

### Tables
- `ItemTypeInfos[type]` (0x08068f50, 40 bytes): baseItems, prefixRegionRanges, prefixes,
  prefixFlags, suffixTierRanges, suffixes, suffixFlags, baseGfx, baseMetadata, u8 baseItemCount.
  Macros: `ITEM_TYPE_INFO(item)`, `ITEM_BASE(item)`, `ITEM_BASE_METADATA(item)`.
- Per base item, three parallel tables indexed by baseIndex:
  - `ItemStatRecord` (12 bytes): nameId (TextId), tier, 4x (stat, s8 value), STAT_NONE = 90.
    Damage stats show as lo-hi.
  - `ItemGfx` (28 bytes): backpack animation, equip icon, hollow icon, lootType (the ground sprite
    and the `LOOT_TYPE_*` drop kind). Build with `ITEM_GFX(Name, LOOT_TYPE_X)`, and use art names
    from `build/include/variables.h` (`<Name>BackpackIconTiles` etc.).
  - `ItemMetadata` (8 bytes): flags (family bit, `ITEM_FLAG_SLOT_*`, `ITEM_FLAG_CLASS_BIT(hero)`,
    NO_PREFIX / NO_SUFFIX, ANCIENT = needs the unlock) and equipSfx.
- Prefixes/suffixes: `ItemStatRecord` tables plus parallel `AffixFlags` (4 bytes). Prefix choice
  comes from `prefixRegionRanges[region]` (lo/hi index per campaign region), suffix choice from
  the suffix tier ranges. All suffix-bearing types share `ItemSuffixes[160]`.
- Uniques: `UniqueItems[53]` / `UniqueGfx[53]`, in three bands (weapons once per character,
  carried passives, repeatable). Each band has a 1-in-N keep roll in `loot_rollBaseItem`.
  `item_descriptions.c` has the unique power texts.

### Drop pipeline (loot.c)
`actor_onDeathDropLoot` -> `loot_rollDrop` (drop chance from `EnemyLootInfos[unitType]`) ->
`loot_generateDrop` (quality roll: gems <= 45 < health < items <= 100) -> `loot_generateItem`
(item type from `LootHeroItemTypeRolls[hero]` thresholds) -> `loot_rollBaseItem` (base tier <=
`g_LootTier`) -> `loot_rollPrefix` (chance = enemy prefixTierChance + lootTier + magic find, region
range minus a difficulty penalty) -> `loot_rollSuffix` (2/3/4% by difficulty, scaled by magic find).
Chests use `unitType == LOOT_UNIT_TYPE_CONTAINER` (0x53).

### Common changes
| Goal | How |
|---|---|
| Drop rates per hero | Edit that hero's `LootHeroItemTypeRolls` row (the Ringwraith's is in `ringwraith_hook.asm`) |
| Rarity / affix chance | Hook `loot_rollPrefix` / `loot_rollSuffix` (the C copy in loot.c is the template) |
| New base item, affix or unique | Extend the table (stock rows copied in plus new rows), repoint ItemTypeInfos (section 5b pattern), and raise baseItemCount if stock rolls should pick it |
| Custom name | Text id >= 0x4000 served from `text_overrides/text_overrides.c` (see `wraithGear_findString`) |
| Name color | `item_affix_formatName` sets `g_PlayerHuds[ACTIVE_PLAYER_INDEX].textColor` (wrapped in wraith_gear.c) |
| Info panel text | `item_descriptions.c` `itemDescriptions_drawInfo` |
| Who can equip | `ITEM_FLAG_CLASS_BIT`, or the `item_getEquipSlot` wrapper |
| Item stats in play | Stock: stat records are applied automatically. Custom effects: sum the equipped slots (`PLAYER(p).inventory.array[0..7]`) per tick, see `wraithGear_refresh` |
| Pickup / filter behavior | `hacks/patches/loot_qol/` (loot filter, auto-collect gems) |
| Health drops | `loot_rollHealthDrop` and the weighted health table in loot.h |

Gotchas:
- Only one patch can hook each of `loot_generateItem`, `item_getEquipSlot` and
  `item_affix_formatName`. They are taken by `wraith_gear.c`, so extend that.
- Literal refs that bypass ItemTypeInfos must be repointed too. ItemSuffixFlags has three
  (`wraith_gear.asm`). Search the ROM for other tables' addresses the same way before
  extending them.
- New rows must never appear in stock rolls unless baseItemCount and the region/tier ranges are
  updated to include them.

## 5b. Hero-only gear

Copy the `wraith_gear` pattern:
- Extended item tables = stock rows copied in by `wraith_gear_tables.asm` plus new rows, with
  ItemTypeInfos repointed. baseItemCount stays stock, so other heroes never roll them.
- Names: text ids from 0x4000 through `text_overrides` findString.
- Drops: the `loot_generateItem` replacement branches on heroId.
- Equip lock: `item_getEquipSlot` wrapper.
- Effects: sum the equipped items each tick (`wraithGear_refresh`) and read them in the kit.
- Only one patch can hook `loot_generateItem` / `item_getEquipSlot` / `item_affix_formatName`.
  A second hero's gear must extend `wraith_gear.c`, or the gear code should first become generic
  (per-hero gear sets).

### Wraith gear layout (wraith_gear.c)

Full item, affix, effect and drop-rate reference: `ringwraith-gear.md`.

Wraith bases per item type, relative to the first wraith base (stock rows come first):

| Type | Bases | Drop slots (5 tiers + uniques) |
|---|---|---|
| Mace | 12 | Mace 0..4 + 5, Maul 6..10 + 11 |
| Sword1H | 12 | Blade 0..4 + 5, Fang 6..10 + 11 |
| Armor (body and helm) | 24 | Crown 0..4, Hauberk 5..9, uniques 10/11, Helm 12..16, Mail 17..21, uniques 22/23 |
| Accessory | 15 | Ring 0..4 + 5, Pendant 6..10, uniques 11..14 |
| Cloak | 12 | Shroud 0..4 + 5, Wrap 6..10 + 11 |
| Gloves | 12 | Gauntlets 0..4 + 5, Grips 6..10 + 11 |
| Shoes | 12 | Greaves 0..4 + 5, Treads 6..10 + 11 |

`WraithDropSlots` picks a slot by weight, then either one of the slot's `uniqueBases` (chance
`3 + g_LootTier / 3`%, 0 when `uniqueCount` is 0) or a tiered base. Tiered bases use tiers 1,3,5,7,9
and only roll up to `g_LootTier`. Affixes use `pickAllowed`: any row whose tier is at most
`g_LootTier + 1`, and the first 2 rows always.

Effects are `WRAITH_FX_*` values (up to 2 per row) plus an optional `WRAITH_UNIQUE_*`. The kit reads
them with `gear(p, WRAITH_FX_X)` (summed over equipped items) and `hasUnique(p, WRAITH_UNIQUE_X)`.
`wraithGear_refresh` rebuilds the sums only when an equipped item word changes.

### Recipe: add a tiered base family (5 items) to an existing type

1. `WRAITH_STRINGS`: add `X(FAMILY_1, "Tier Name")` .. `X(FAMILY_5, ...)` at the end of the list.
2. Raise the type's `*_COUNT` by 5 (by 6 with a unique).
3. Append 5 `RECORD2(FAMILY_n, tier, STAT_A, v, STAT_B, v)` rows to the type's `Wraith*Items`. Use
   tiers 1, 3, 5, 7, 9. Stat names are in `include/stats.h` (there is no STAT_DEXTERITY, use
   STAT_ACCURACY).
4. Append 5 `ITEM_GFX(ArtName, LOOT_TYPE_X)` rows to `Wraith*Gfx`. Art names: grep
   `carved/data/item_tables/<Type>Tables.c` and `UniqueTables.c`. The name needs both
   `<Name>EquipIcon` and `<Name>HollowIcon` symbols.
5. Append `META5(slot flags, sfx)` to `Wraith*ItemFlags` (copy the flags of the same slot).
6. Append 5 `FX1(...)` / `FX2(...)` rows to the type's `*Effects` array.
7. Add a `WraithDropSlots` row `{weight, typeIndex, firstBase, uniqueCount, {uniqueBases}}` and take
   the weight from other slots so the total stays 100.
8. Build. A static assert names any table you missed.

### Recipe: add a wraith unique

1. `wraith_gear.h`: append `WRAITH_UNIQUE_X = n` before `WRAITH_UNIQUE_COUNT` and bump the count.
2. Name string, then one row in each of the 4 tables like above, with `META(flags | WRAITH_UNIQUE_FLAGS,
   sfx)` (no affixes) and `UNIQUE(X)` in the effects array.
3. Add its base index to a drop slot's `uniqueBases` (max `WRAITH_SLOT_UNIQUE_MAX`, 4) and bump
   `uniqueCount`.
4. `UniqueDescriptions[WRAITH_UNIQUE_X]`: the text in the info panel. Keep it under about 60 chars.
5. Implement it in `ringwraith_kit.c` with `hasUnique(playerIndex, WRAITH_UNIQUE_X)` at the right
   hook (melee damage, on hit, pulse, a skill callback).

### Recipe: add a wraith effect (WRAITH_FX)

1. `wraith_gear.h`: append `WRAITH_FX_X = n` before `WRAITH_FX_COUNT`.
2. `EffectLabels[WRAITH_FX_X]`: the text after the number, e.g. `"% Unseen Damage"`. Frame values
   display as seconds only if `appendEffect` lists the effect next to TERROR_STUN/UNSEEN.
3. Read it in the kit with `gear(playerIndex, WRAITH_FX_X)`.

### Recipe: add a wraith prefix or suffix

1. String, then append to the END of `WRAITH_PREFIX_RECORDS` (or `WraithItemSuffixes`), even if its
   tier is lower than the last row. `pickAllowed` does not need tier order.
2. Bump `WRAITH_PREFIX_COUNT` / `WRAITH_SUFFIX_COUNT` and append the matching row to `PrefixEffects`
   / `SuffixEffects`. All 5 prefix tables share the records macro.
3. The `u8` index plus stock counts (156 max) must stay below 0xff (ITEM_AFFIX_NONE).

## 6. Test loop

1. Build (`hacking-recipes.md`), copy to a fresh ROM name, and load it in mGBA.
2. Use the Debug page for max level and skill points, then check both skill tree pages: buy, the
   level display, required levels and descriptions.
3. Leftover tiles: a page with fewer than 9 passives must clear the leftover tiles and their
   `g_SkillTreePassiveTileSkillIds` entries. Otherwise the other page's skills stay on screen and
   can be bought from there (`skillExpansion_treeDrawPassiveGrid` does this).
4. Things that are easy to break: page 2 tree state (the swap in `skill_expansion_tree.c`), saves
   (`skill_expansion_save.c` must cover any new per-player state) and co-op (the page can't be
   turned in co-op).
5. Gear: farm drops (a late chapter rolls every tier), check new names, icons, the info panel effect lines,
   that uniques have no affixes, and that an old save's gear still has the same names.

## 7. Custom skill icons (user art)

The user draws icons as one upscaled mockup of the skill screen: 3x3 passive grid on the left,
3/2/1 active slots on the right (example: `Desktop/lotr/new icons/wraith skill icons.png`, 1858x846,
7.5x GBA scale). The user lets us choose which picture fits which skill.

Icon format: 32x24, 4bpp, 4x3 tiles row major, in the one 16-color palette all skill icons share
(0 transparent, 1 white, 2..6 gold/orange/red/brown, 7 near black, 8..12 greens, 13 navy, 14/15
grey green). The asset is `u8[0x184]`: header `{0x10, 0, 0x80, 0x01}` plus 384 bytes of tiles.
The frame ring and corners are identical in every stock icon, so the tool copies them from stock
and takes only the circle interior from the art.

Steps (needs `make carve` output in carved-local):
1. Look at the mockup and decide a skill name per slot. Slot order: 9 passives row by row, then
   the actives top row left to right, middle row, bottom.
2. Slice into PNGs (one per slot, `-` skips a slot):
   `uv run --with numpy python hacks/tools/skill_icon_art.py slice "<mockup.png>"
   hacks/patches/<hero>/icons --names a,b,c,...`
   A mockup of a different size works if the layout is the same (positions scale by width).
   An icon sheet (rows/columns of framed icons, any count) slices with `--grid X,X,..:Y,Y,..:SCALE`:
   frame top-left pixel per column and per row, and the upscale factor. Slots run row by row, use
   `-` in `--names` for unused ones. Frodo example (`new icons/Frodo type icons.png`, 9x4 sheet):
   `--grid 13,206,397,604,796,987,1194,1388,1579:94,294,494,696:5.69`.
   Sheets whose blocks have different column offsets: slice each block in its own run into the
   same folder. Sam/Smeagol sheets (`new icons/Sam icon.png`, `Smeagol.png`, 1905x826, 7.0x, frame
   height includes a pip row that is ignored): left 4x3 `42,277,502,718:43,277,513:7.0` (12th slot
   is a half-width lantern/eye, skip), right row 1 `1114,1331,1545:43:7.0`, right rows 2-3
   `1100,1318,1533:277,513:7.0` (4th right column is cut off, skip).
   Three-hero sheet (`new icons/Gimli-Legolas-Gandalf.png`, 2172x724, three 5x3 blocks, frames
   about 135x121 so not 4:3): scale by the circle, not the frame (circle ~112x103 px vs the stock
   ~26x22, so 4.5). Rows `157,317,477`, columns Gandalf `36,172,308,443,579`, Gimli
   `751,884,1018,1151,1284`, Legolas `1453,1589,1725,1861,1996`, `:4.5 --backdrop 62`, one run per
   block. Never commit stock icon copies as placeholders: the repo is public and must hold no game
   art. Use a passive's stock `iconPassiveSkillId` (drawn from the ROM) until user art exists.
   Grey art (Gollum skin, trowels, rope): `--backdrop 62`. By default greys with mean brightness
   under 140 count as stone backdrop and turn transparent, which erases grey subjects. These sheets'
   circle backdrop is about (52,50,51). The palette has no neutral grey, so greys land on the pale
   green/grey-green ramp (8, 14), which page 2 recolors per build anyway.
   Slice everything into a scratch folder first, contact-sheet it, then copy the chosen PNGs into
   `hacks/patches/skill_expansion/<hero>_icons/` under the skill's snake_case name.
3. Open the PNGs and check them. Hand-fix any pixel in an indexed-PNG editor if needed (keep the
   palette).
4. Generate C: `uv run python hacks/tools/skill_icon_art.py build hacks/patches/<hero>/icons
   hacks/patches/<hero>/<hero>_icons.c --prefix <Hero>Icon`. This writes `<hero>_icons.c/.h` with
   one `<Hero>Icon<PascalName>` array per PNG (`will_of_sauron.png` -> `RingwraithIconWillOfSauron`).
   Never edit the generated files, rerun build.
5. Hook them up:
   - Page 1 of a custom kit: the kit's `icons[6]` array (`RingwraithActiveIcons`).
   - Page 2 actives: `.icon = <Hero>IconX` in the `SkillExpansionActiveDef`.
   - Elite passives: `.icon` in `SkillExpansionElitePassives` (the Ringwraith uses the
     `RINGWRAITH_ICON(Name)` macro so the file builds without the ringwraith patch).
   - Stock passives on a custom hero cannot take a custom icon yet.
6. Build and look at both tree pages and the HUD.

Build colors (page 2): page 2 art is drawn through palette colors 8..13, which get a ramp per build
(`BuildArtRamps` in skill_expansion_hud.c: gold = Good, violet = Evil, green = Tricks, darker when
uncastable). Blue art quantizes mostly into 8..13 so it recolors well. Tag a skill with
`skillExpansion_activeBuild` (per row table, e.g. `FrodoActiveBuilds`) and `skillExpansion_passiveBuild`
(switch on passive id) in skill_expansion_content.c. Untagged (NONE) keeps the blue page 2 ramp.
Palette banks (trap): every tree tile draws its icon with BG2 palette bank 4, so one ramp loaded into
bank 4 recolors the whole page (the last tile drawn wins). `tintTileForBuild` (skill_expansion_tree.c)
loads each build's ramp into its own free bank (`BUILD_PALETTE_FIRST` 9 + build - 1, so 9 Good,
10 Evil, 11 Tricks) and rewrites the tile's 4x3 icon map entries from bank 4 to that bank. BG2's map
is VRAM itself (screen block 29), so the direct write sticks. Tree scene banks: 0..3 menu, 4 icons,
5 level digits, 6 unique tile, 7..8 disabled tile/level, 9..15 free (12..15 left for new builds or
other heroes). A 4th build = add a ramp to `BuildArtRamps` and an enum value, it takes bank 12.
Columns: passive tiles sort by first required level into the 3x3 grid, so give each tier levels
in column order (Good < Evil < Tricks: Frodo 10/11/12, 14/15/16, 22/23/24). Active tiles are a fixed
3/2/1 pyramid in defs table order, so order the defs to put each build under its column.

## 8b. Designing a hero resource (lessons from Frodo's Burden)
- Check what actually feeds a stock meter before balancing around it (Ghidra xrefs to its setter). Stock
  corruption only rises from The One Ring and drains by itself, so "keep it low" builds were free.
- Every build should fight the resource: give it passive pressure (gain over time near foes, gain from a
  foe family's hits), make one build's skills the main way to lower it, and let another build cash it in.
- Prefer hero-tick control over stock patches: hold a stock countdown full, then apply your own rules.
- Show the resource on the HUD (section 9) and announce thresholds with `skillExpansion_feedback`.
- Recipes and addresses: hacking-recipes.md "Stock corruption (Burden) system" and "Enemy identity".

## Borrowed baseRow blocks casts (trap)
- `activeCanCast` refuses a row while its baseRow's stock action-state bit is set. Buff rows like
  Numenorean Will (bit 3) and King's Command keep that bit set for the buff's whole duration, so an
  elite active borrowing them can't be cast while the stock buff runs.
- Fix: give the def a `.policy` with the borrowed pose (`baseRow + 1`) and
  `SKILL_EXPANSION_RULE_NONE` for nativeCooldownRow and requirementRow. See `NumenoreanWillPosePolicy`
  in skill_expansion_content.c. Instant stock rows (Knife Toss) don't need it.
- Test: cast the stock buff, then the elite active. `actionStateFlags` shows the bit.

## Page 2 slot limits
- Tree active slots are a 3/2/1 pyramid. Page 2 swaps rows 7..11 into the top 5 slots. The bottom slot
  shows row 12 if the hero's 6th new active def (`activeDefs[5]`) is non-NULL, else row 5 (Herbal) on
  both pages. `pageActiveCount()` in skill_expansion_tree.c decides it.
- To give a hero a 6th page 2 active: add the def as the 6th entry of its new-actives list, a 6th
  `SkillExpansionBuild` entry, and nothing else. Frodo's Put On the Ring is the example.
- Row 6 must stay ACTIVE_SKILL_NONE: stock code uses it as the "no skill" row.
- Saves: the 32-byte row record stores only rows 7..11 (`ROW_RECORD_ACTIVE_COUNT`). Row 12 lives in the
  ID record's `extraActives[]` (taken from reserved bytes), so old saves load with row 12 at level 0.
  Each hero's `LegacyActiveIds` row has 13 IDs (`0xN0d` for row 12).
- Trap: validate_abilities.py derives the LegacyActiveIds stride from the symbol size. If you add more
  rows, it follows automatically, but the 144-byte ID record has only 6 reserved bytes left.

## Two-form hero (Gandalf's Grey and White, v36)
- One def per row, the resolver branches on form (`isWhite(p)`). No second def table needed.
- Per-form Spirit costs: rewrite `skillExpansion_players[p].activeSkills[row].spiritCost` every tick.
  Skip it while `skillExpansion_treePage != 0` (the tree has rows 0..5 and 7..12 swapped). Discounts:
  compute from a const base table, never from the live cost, or they compound.
- Per-form icon color: return a build from `skillExpansion_activeBuild` based on the active player's form.
  New builds need a ramp pair in `BuildArtRamps` (hud.c) and the bound in `skillExpansion_buildArtRamp`.
- Input that is not a skill slot: add a `skillExpansion_<hero>Input(p)` call in `skillExpansion_inputUpdate`
  (smeagol.c). Hold-R (without L) is free: stock acts on the R press (loot) and L+R (quick slot).
- Temporary stat changes (no regen, haste): edit `PLAYER_STAT` and save the old value. Stats rebuild on load
  and level up, so re-apply each tick and only undo if the stat still holds your value.
- Every new hook in elite.c needs a stub in hacks/tests/aragorn.c, and input hooks one in hero_moves.c.

## Timing hero with nocked arrows (Legolas "The Glint", v1)
- Pattern: actives do not act on cast. The resolver stores the row ("nock") and returns SKILL_CAST_SUCCESS.
  The effect fires later from the hero tick. One shared resolver for all rows, branching on `castingActiveSkill`.
- Spirit already paid: `skillExpansion_runtime[p].paidSpirit` inside the resolver. Refund with `player_addSpirit`.
- Refusal text: `canCast` returning FALSE shows the stock message. For your own text, add a hook to
  `skillExpansion_rejectCast` (runtime.c, see `skillExpansion_legolasRejectMessage`). tests/cast_timeline.c needs a stub.
- Reading a stock move's timing: hook input in `skillExpansion_inputUpdate`, test `actor->actionState` and the frame
  the stock state body tests (Overdraw: state 0x60, `previousAnimationFrame == 9`, NOT currentAnimationFrame).
  Queue the event in input, resolve it in the tick.
- Weapon gate: test the family, not the item type (unique items have ITEM_TYPE_UNIQUE):
  `mainHand.d.itemType != ITEM_TYPE_EMPTY && ITEM_BASE_METADATA(mainHand).flags.d.familyBow`.
- Boost only stock projectile hits: in the hero's damage hook, check
  `hackLib_currentDamageSource()->kind == HACKLIB_DAMAGE_PROJECTILE`. Your own ability hits use HACKLIB_DAMAGE_ABILITY.
- Don't reuse a stock passive's name for a new passive (Legolas Keen Eyes became Elf-Sight).

## 9. HUD meter for a hero (corruption bar pattern)
See hacking-recipes.md "HUD meter" and "Gameplay HUD layout". Frodo's Burden bar is the template:
`skillExpansion_frodoDrawBurdenBar` in skill_expansion_frodo.c.

## 8. Skill visuals (FX)

Read `fx-catalog.md` before giving a skill a visual. API: `hacks/lib/fx_kit.h` (`hackLib_fxBurst`,
`hackLib_fxTimed`, `hackLib_fxAt`, `hackLib_fxEnemyAnim`, `hackLib_fxHold`). Preview shapes,
palettes and enemy anims with `hacks/tools/fx_palette.py`. Effects can fail to spawn (NULL), so
the skill logic must never depend on them.
