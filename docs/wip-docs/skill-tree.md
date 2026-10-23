# Skill tree scene (SCENE_ID_SKILL_TREE) RE notes

Placeholder rows `scene_skillTree_unknown<addr>` added to config/functions.cfg (0x080418fc..0x08042864).

## Scene state
- `g_SceneCurrent.args[1]`: cursor index within the current grid.
- `args[2]`: 1 if the player had skill points on entry.
- `args[3]`: grid. 1 = active skills (6 rows), 2 = passive skills (9). Starts at 2.
- 0x030069e0 (u8): next quick-select slot to assign (round robin 0..2).
- 0x030069f0 (Actor*[3]): quick-select marker sprites.
- 0x03002150 (Actor*): cursor sprite.
- 0x03006a10 (u8[9]): passive grid slot -> PassiveSkillId, filled sorted by required level.

## Tables
- 0x0806e2c0: passive grid tile positions, 8 bytes/entry (x u32, y u32).
- 0x0806e308: active grid tile positions, same layout.
- Nav tables (s8 per cell, negative = switch grid, target = ~value):
  right 0x0806e33e (active) / 0x0806e359 (passive), left 0x0806e34a / 0x0806e36b,
  up/down in 08041bc4 / 08041c24.
- 0x0806e374: quick-select marker anims, 0x10 per slot.
- 0x08287488: level-digit icon assets indexed by level.

## Functions
- 080418fc SELECT: active grid: toggle activeSkillCycleEnabled[row], or if optionsFlags & 8 assign to a
  quick-select slot via 08042548. MP msgs 0x2c (toggle) / 0x2e (quick-select, packs 3 slot bytes).
- 08041a24 d-pad: dispatches to 08041b04 (R) / 08041b64 (L) / 08041bc4 (down) / 08041c24 (up), moves cursor.
- 08041c84 draw passive tile, 08041da0 draw active tile (greys out if cycle-disabled).
- 08041f78 A = buy: needs skillPoints, required level (080422f0) <= level+1, then skill_active_addLevels /
  skill_passive_addLevels, skillPoints--. MP msgs 0x22 active / 0x21 passive.
- 080421f8 draw description text for cursor.
- 080422f0 required level of next purchase: ActiveSkillRequiredLevels per hero [6][6], PassiveSkillRequiredLevels. -1 = maxed.
- 080423e8 create cursor sprite. 08042474 create 3 quick-select markers.
- 08042548 assign quick-select slot (swap if skill already in another slot).
- 0804267c skillPoints--. 0804269c toggle cycle (MP receive side probably).
- 08042704 draw skill point count. 08042780 A-press wrapper -> 08041f78.
- 0804279c/080427c4 build passive grid (levels 0..20). 08042854/08042864 draw the 6 active tiles.
- 08042674, 08042678: empty stubs.

## Hardcoded slot counts
- Quick-select = 3: Player.quickSelectActiveSkills[3], loops `< 3` in 08042474 and 08042548, wrap `> 2`,
  MP msg 0x2e packs exactly 3 bytes, input side is L+A / L+B / L+R.
- Active rows = 6: activeSkillCycleEnabled[6], activeSkillLevelsPurchased[6], loop `< 6` in 08042864,
  ActiveSkillRequiredLevels [6][6], nav/position tables sized for 6.

## Screen layout (4x4 tiles, x/y in BG tiles)
- Passive 3x3 grid at x {1,5,9}, y {3,7,11}. Active pyramid: (17,3) (21,3) (25,3) (19,7) (23,7) (21,11).
- Description box rows 16..19. Screen is full; more tiles need pages or a new layout.

## Skill expansion: what indexes skill slots generically (Ghidra field xrefs)
Player is a fixed 0x1ac; slot arrays are inline. activeSkillLevels[8] has 2 spare bytes (6, 7).
Player.activeSkills and passiveSkillIds are ROM pointers, so bigger per-hero tables are easy.
- activeSkillLevelsPurchased: player_initItemsAndHollow, scene_backpack_enter, save_(de)serializePlayer,
  skill tree, player_resetActiveSkills, player_initHeroLoadout, skill_active_addLevels/addPurchasedLevels.
- activeSkillCycleEnabled: save, player_resetActiveSkillControls, skill tree, skill_active_handleCycleCastModeInput.
- activeSkillLevels: 55 fns, but per-skill callbacks read a fixed row. Generic ones: canCast, hasLevels,
  applyLevels, temporary-level fns, handleCycle/QuickCastModeInput, init/reset, combat_* (fixed rows?).
- activeSkills ptr: actor_player_tick, skill_active_cast/canCast/getLeveledValue, temp-level fns, init.
- passiveSkillIds / LevelsPurchased / LevelsApplied: skill_passive_* (all in src/c), save, init, skill tree.
- ACTIVE_SKILL_HERBAL_HEALING = 5 and ACTIVE_SKILL_NONE = 6 sentinels: new rows go after 5 and NONE must move.
- Save: SaveSlot.skillLevelData is 3-bit packed (passives 9 + actives 0..4). mod_save has versioned migration.

## Expansion plan (decided 2026-10-20)
Target per hero: 10 real actives + Herbal Healing, 18 passives. Stock tree is page 1, new page 2 uses the
same layout (3x3 passives, 6-tile active pyramid with 1 spare tile). New actives: mix of reused callbacks
and brand new effects.

Row numbering: keep 0..4 stock, 5 Herbal Healing, 6 = ACTIVE_SKILL_NONE (null-callback row), new actives
at 7..11. Keeps every stock `== 6` / null-callback check valid.

Phases:
1. Backend: `skill_expansion` patch. Extended per-hero ActiveSkill[12] + passive id[18] tables in modcode
   rodata, Player.activeSkills / passiveSkillIds repointed in player_initHeroLoadout. Shadow arrays in mod
   RAM for slots past the stock arrays, accessor helpers, hookReplace the generic C skill fns with copies
   that loop the larger counts. Test by giving one hero a reused-callback 7th active.
2. Save: new mod_save version storing extended levels, cycle flags. Migration fills zeros.
3. Skill tree page 2: page toggle, page-aware position/nav tables, drawing, purchase, required levels.
4. Content: passive records (PassiveSkills extension + text), active rows, icons, descriptions.
5. Brand new active effects, one at a time.
Still to RE: save_(de)serializePlayer, player_initHeroLoadout, player_resetActiveSkills,
player_initItemsAndHollow, scene_backpack_enter, skill_active_hasLevels, HUD active-skill icon draw.

## Phase 1 status (2026-10-20)
Patch `hacks/patches/skill_expansion/` built. Per-player RAM tables (activeSkills[12], icons[12],
passiveSkillIds[18]) + shadow levels, hookReplace of all generic skill fns, pose patch at
actor_player_tick + 0x17ac. MODCODE_BSS grown to 0x0203f800 (players struct 0x4b8 B).
Reused stock callbacks run through skillExpansion_castStockCallback, which swaps the new row's level,
table row and castingActiveSkill into def->baseRow. Persistent effects that re-read the base row's
level later (Dwarven Rage tick etc.) still see the base level.
Test content: Aragorn row 7 = Sweep copy, SKILL_EXPANSION_TEST_GRANT_NEW_ACTIVES = 1 grants level 1.
Not saved yet (Phase 2): shadow levels, rows 7..11 cycle flags.

## Phase 3 status (2026-10-20): page 2 built, untested on hardware

- skill_expansion_tree.c. Turning the page swaps active rows 0..4 <-> 7..11 and passive slots 0..8 <-> 9..17
  (tables, icons, levels, cycle flags), and permutes quickSelect/cycleSkillIndex. The exit hook swaps back.
- Turn page: RIGHT on active tiles 2/4/5 -> passive 0/3/6, LEFT on passive 0/3/6 -> active 2/4/5.
  Disabled in coop / MP_MSG_FLAG_2.
- Replaced: handleDpad, drawPassiveTile/drawActiveTile (hookReplaceR3), drawDescription, getRequiredLevel,
  createQuickSelectMarkers, assignQuickSelect, exit. Off-page markers are parked at tile (31,21).
- Page 2 tile 8 has no unique check mark. Empty rows (NULL callback) draw only the frame and can't be bought or bound.
- No page label yet. Needs a screenshot to pick a free spot.
- Untested: whether gfx_fillBgRect(2,...,0) blanks an empty tile cleanly.
- Content: page-2 passive ids are filled in skill_expansion_content.c.
- Still pending: Phase 2 save (shadow levels aren't saved), new descriptions/text, real new actives.

## Phase 2 (save) done
- mod_save region 5 (save_layout.h SKILL_EXPANSION_SAVE_OFF): 32-byte record per slot (page*4+slot).
  tag 0x5e, byte-sum 0, slotChecksum must equal SaveSlot.d.checksum, else shadows load as 0.
- Written from modSave_writeSlot (save_slots.c, HACK_skill_expansion block).
- skill_expansion_save.c replaces scene_saveLoadMenu_loadSlot / loadPeerSlot. Peer gets empty page-2 state (not sent over link).
- Quick-select rows stored as full bytes in the record, sanitized after loadSlotState.
- SKILL_EXPANSION_TEST_GRANT_NEW_ACTIVES now 0.

## Page-2 tint
- tintAssetPalette in skill_expansion_tree.c: after a page-2 tile draw, re-uploads hero bg / icon / disabled bg palettes with R<->B swapped (banks 4 and 7). Page 1 redraw restores them.

## Page title
- skillExpansion_treeDrawPassiveGrid (replaces drawPassiveGrid, called by enter + turnPage) redraws the BG3 title as '< Skills Page N >' over the stock '@1 - Level @2' (text 0x113). '<' '>' unconfirmed in font.
- Title now 'Skills - Page N' (no angle brackets). Edge arrows: 8x16 tiles written into BG3 charbase 2 tile 0x2f0, map at cols 0 (hflip) and 29, rows 8-9, color 3. Hidden when !canTurnPage.

## Skill point economy
- Option 2 chosen: 2 skill points per level from displayed level 21 (internal level >= 20).
  Stub skill_expansion_level_up_points_hook (skill_expansion_hook.asm), patched at player_awardXp+0x1fe (12 bytes, returns to +0x20a). Verified bytes in built ROM.
- Budget: ~49 level points + 30 bonus + 15 shop = ~94 vs ~132 to max both pages.

## Page 2 = "Elite" skills (approved design, Aragorn first)
Required levels 21..40. All unique, mechanic-driven.
Actives: Flame of the West (on-hit light burst AoE, kills extend), Ranger's Ambush (stealth, first hit guaranteed crit x2+50%/lvl + stun),
 Oath of the Dead (detonate Call of the Dead ghosts: fear + dmg, else raise 1 weak ghost), Palantir's Challenge (pay 20% HP, all on-screen foes +25%+5%/lvl dmg taken 10s),
 Return of the King (ultimate: 0 spirit cost + skills +1 level for 10s+2s/lvl).
Passives: Strider's Riposte (dodge -> free Sweep 10%/lvl), Heir of Isildur (+5%/lvl vs foes with more max HP), Tracker (+3/lvl vs unaware foes),
 King's Presence (stuns +0.5s/lvl, +2 dmg/lvl to stunned), Healer's Touch (Herbal Healing +10 spirit/lvl, heals partner), Captain of the Dead (ghosts +3s/lvl, stronger),
 Ranger's Rest (3s no damage -> 2%/lvl max HP/s), Momentum (kill chain +1 dmg stacks, cap 2/lvl), Lord of the Dead (unique: 10% on kill raise Ghost Warrior).
Needed hooks: on-hit, on-kill, on-dodge, damage dealt/taken, foe-awareness, ghost tracking, Silent Stride stealth, new skill name/description text.

## Aragorn elite implemented (built 2026-10-20, untested in game)
- skill_expansion_elite.c: hookReplace wrappers of combat_computePveDamage / resolvePveAttack / applyPveRangedHit /
  resolveEnemyAttackOnPlayer / actor_player_tick. Originals via ORIGINAL_THUNK (replays the shared 8-byte prologue,
  jumps to fn+8; game_symbols.ld symbols already carry |1).
- Herbal Healing (all heroes) and Aragorn CotD callbacks wrapped in buildTables (skillExpansion_wrapStockCallbacks).
- Per-player EliteState in BSS (0x58 total), reset in initHeroLoadout. BSS now 1305/2048 bytes.
- Elite passive IDs 35..43 live in SkillExpansionElitePassives (content.c), no stat records.
- Ghost = ACTOR_TYPE_NPC, variant NPC_KIND_SIEGE_B (0x1a), parentActor = owner. Spawn 0x08035864(kind, &pos, owner,
  target, lifetime=180). as+0 target, as+4 owner, as+8 u16 lifetime, as+0x11 damage. 0x080013a8(playerActor, cursor)
  = next on-screen enemy in g_ActorListMain.
- Riposte triggers on any failed enemy attack (miss/parry/dodge/block), not just dodge.
- Unverified: ACTOR_FLAG_23 + spriteFlags 0x20 stealth on Aragorn, that iterateInRadius returns only enemies,
  Oath lifetime=1 despawn path decrementing comboState[0].

## Page-2 icon colors (2026-10-20)
- Icon art = palette colors 8..13 (green ramp). Frame = 2..7. HUD skill icon: BG0 palette bank 1, tiles 0x56,
  loaded by player_tickHud via gfx_loadBgAsset(0, icons[row], 0x56, 1, 0, 0).
- 0x0802939c named player_updateHudSkillPalette: stock loads 0x087815d4 (green) / 0x08782330 (red, uncastable),
  PlayerHud flag bit 9 = red loaded. Replaced by skillExpansion_hudUpdateSkillPalette (skill_expansion_hud.c),
  which writes only colors 8..13 of bank 1 and compares palette RAM, picking stock green/red or page-2 blue/magenta.
- Tree page 2: tintAssetPalette keeps R<->B frame swap, art 8..13 = SkillExpansionPage2ArtRamp (blue-grey when cycle-disabled).

## Frodo elite page (implemented 2026-10-21 in skill_expansion_frodo.c, untested in game; ROM ROTK-Mod-Frodo-v12.gba)
Identity: Burden (0..100, per-player EliteState). No other hero has a resource that is both buff and risk
(Ringwraith's Ring meter was cut). Avoid: stealth-crit (Ringwraith Assassin, Aragorn Ambush), light/fear
bursts as pure damage (Gandalf, Flame of the West), plain shields (Gandalf, Gimli), melee role.
Sam's page 2 should be Cookpot / Samwise the Strong / partner protection so the two don't blur.

Burden: One Ring cast +25, Ring's Persuasion +15, Mithril block +8. Decays 2/s after 5s with no Ring skill.
>= 50 Heavy. 100 Consumed: -1% max HP/s, no decay until a purge or 10s pass. HUD display TBD.

Passive grid = one column per build, row = tier.
| tier | Embrace | Resist | Misdirection |
|---|---|---|---|
| 1 | Weight of the Ring: +2%/lvl dmg per 10 Burden | Strength of Will: Burden gain -10%/lvl, +spirit regen while Burden < 25 | Riddles in the Dark: confusion +1s/lvl, confused foes take +5%/lvl from other foes |
| 2 | Corrupting Touch: while Heavy, hits heal 2%/lvl of dmg (offsets Consumed drain) | Sting's Warning: Sting glows with a foe in range and Burden < 50; hits while glowing refund 2 spirit/lvl | Cunning Snares: snared foes are confused on release |
| 3 (unique) | Ring-bearer's Resolve: once per stage, lethal hit -> 1 HP, Burden 100, One Ring auto-cast | Unwavering: 10s at 0 Burden -> next hit guaranteed crit + stun, next Phial free | Stolen Glory: enemy-on-enemy kills give you full XP, 50% food/potion drop |

Actives:
- Mithril Coat (Embrace): blocks next 1+lvl hits for 15s, each block +8 Burden.
- Phial of Galadriel (Resist): purge all Burden; light burst dmg 10 + purged*(0.5+0.1/lvl), fear 2s. Tiny burst at 0.
- Whispers of the Ring (Misdirection): confuse a foe; foes it hits while confused get confused too (chain 1+lvl). +10 Burden.
- Elven Decoy (Misdirection): decoy at your spot for 5+1/lvl s, nearby foes target it; on expiry, confuses foes around it.
- Mount Doom (ultimate, branches on Burden): >= 50 "Claim It" = invisible, Ring skills free, drain paused 10s+2s/lvl;
  < 50 "Cast It Into the Fire" = purge, heal 30%+5%/lvl, stun foes on screen.

Required levels (Frodo only, user 2026-10-21): midgame (~10) to ~25. Other heroes keep their curves (do NOT touch Aragorn or Ringwraith).
To RE: Ring's Persuasion confusion internals (duration, chaining), enemy target selection (decoy),
XP credit for enemy-on-enemy kills, enemy-vs-enemy damage path, calling One Ring outside a cast.

### Frodo as built (supersedes the draft above where they differ)
- Burden IS the stock corruption meter (Player.corruption 0..100, player_addCorruption). The stock HUD
  already draws a corruption gauge while corruption != 0. Added: sprite tint toward dark violet
  (strength corruption*10/100) and shadow wisps at 91+. Stock: One Ring +50, tier 1 enemy buff at 51,
  Nazgul at 91, decay 1 per 25 ticks without the ring.
- All 5 actives use baseRow Knife Toss (Snare's rule needs Snare level > 0; page-2 casts never set
  cooldown bits so sharing is safe). Levels: Mithril 10, Phial 11, Whispers 12, Riddle 13, Mount Doom 16.
- Passive first levels: tier 1 10/11/12, tier 2 15/16/17, tier 3 22/23/24 (Embrace/Resist/Misdirection).
- Stock Persuasion is wrapped: +10 Burden on success, Riddles in the Dark extension on newly turned foes.
- Test list: tint visible and clears at 0; Mithril blocks N hits; Phial damage scales with Burden;
  Whispers turns foes (fail message when none); Riddle stuns; Mount Doom both branches and costs restore;
  Resolve once per mission; Unwavering after 10s at 0 Burden; Strength of Will reduces One Ring's +50.

## Frodo page 2 rework v2 (DRAFT 2026-10-21, awaiting user sign-off)
User feedback on v12: page 2 is barely noticeable in game. Wants a straightforward Good / Evil /
Misdirection split, with Evil raising Burden/corruption.
Why v12 reads as invisible: page-2 icons are page-1 icons tinted blue; cast FX are single unpaced
stock bursts that flash by; most passives are hidden percentages; actives don't map to builds
(Mithril was Embrace, Riddle Game was a plain stun, Mount Doom branched invisibly).

Meter: Burden = stock corruption (one meter, the stock HUD gauge). Evil fills it, Good drains it,
Misdirection ignores it (drop v12's +10 on Ring's Persuasion).
Colors: Good = WHITE/GOLD/GHOST, Evil = SHADOW/NIGHT, Misdirection = GREEN/GOLD.
Every cast uses a paced castFx (Ringwraith pattern), every passive proc shows FX + sfx.

Actives (rows 7..11):
| row | build | skill | effect |
|---|---|---|---|
| 7 | Good | Phial of Galadriel | purge corruption, held light zone 6s+1s/lvl: foes inside pulse dmg + slowed/stunned on entry, size/dmg scale with purged |
| 8 | Good | Elbereth! | starlight burst: stun nearby foes, heal self + partner, x2 vs Nazgul/wraith foes, -20 corruption |
| 9 | Evil | Shadow of the Ring | violet cone blast (targeting cone) dmg = base + corruption scaling, +15 corruption |
| 10 | Misdirection | Sow Discord | nearest 2+lvl foes turn on each other (persuasion), gold sparkle ring over each |
| 11 | Misdirection | Smoke and Mirrors | blink ~3 tiles back (hackLib_findLanding), cloak FX held at old spot, foes near it stunned/confused 2s. Stretch: real decoy foes target (needs enemy target-selection RE) |

Passives (column = build, row = tier):
| tier | Good | Evil | Misdirection |
|---|---|---|---|
| 1 | Strength of Will: corruption gain -10%/lvl, white spark when resisted | Weight of the Ring: +2%/lvl dmg per 10 corruption, sprite darkens to full strength, violet spark on hits at 51+ | Riddles in the Dark: turned foes +1s/lvl, sparkle marker |
| 2 | Sting Glows Blue: blade tints blue while a foe is near (fxTintSlot on Frodo's blade ramp, RE indices), glowing hits +2/lvl dmg + spirit refund | The Ring's Hunger: at 51+ or wearing the One Ring, hits heal 3%/lvl of dmg | Cunning Snares: page-1 Snare victims are turned on release |
| 3 | Unwavering (unique): 10s at 0 corruption -> radiant aura, next hit x3 + stun, next Phial free | Claim It (unique): at 100 corruption or once per mission on a lethal hit -> Shadow form 10s (full dark tint, wisps, skills free, +50% dmg), then corruption 60 | Precious Lies (unique): Sow Discord has no target cap, turned foes' kills give you XP |

Cut from v12: Mithril Coat, Riddle Game, Whispers (becomes Sow Discord), Mount Doom (Claim branch becomes the Evil capstone),
Corrupting Touch, Stout Heart, Turn Them Against, Ring-bearer's Resolve (folded into Claim It).
Passive IDs are append only: reuse the v12 Frodo IDs by renaming in place (they were never in a released save
the user cares about? confirm), or append new IDs.
Levels stay 10..25 (Frodo only). Icons: 5 new actives + 9 passives need art (custom-skill-icons pipeline).
Corruption bar (user asked 2026-10-21): dedicated Frodo-only HUD bar, always shown for Frodo (empty = clean, which Good wants).
Marks at 51 (Heavy, enemies buffed) and 91 (Nazgul). Fill color gold -> violet -> black by tier, flash on gain/purge, pulse at 91+.
Unverified: what the stock "corruption HUD actor" (spawns the Nazgul at tier 3) actually draws. RE it in player_tickHud
(0x08028aec) / game_tickFxAndHuds (0x08033190) before deciding to restyle it or draw our own. Needs a HUD screenshot for free space.

## Frodo page 2 v2 as built (2026-10-21, ROM ROTK-Mod-Frodo-v14.gba)
The draft above was approved and built, with these differences:
- Cunning Snares became Tricksy Hobbit: hits have 4%/lvl to turn the foe for 3s.
- Passive ids renamed in place: CORRUPTING_TOUCH -> RINGS_HUNGER, STOUT_HEART -> STING_GLOWS_BLUE,
  TURN_THEM_AGAINST -> TRICKSY_HOBBIT, RING_BEARERS_RESOLVE -> CLAIM_IT.
- Claim It: at Burden 100 (or once per mission on a lethal hit) 10s of free skills, +50% dmg, dark tint;
  Burden drops to 60 after.
- Precious Lies: Sow Discord has no target cap and x1.5 duration (no XP share).
- Smoke and Mirrors is the simple version (blink back 40 px, cloak held at origin, nearby foes turned
  or stunned). No decoy.
- Levels: actives Phial 10, Shadow 11, Sow 13, Elbereth 12, Smoke 14 (+3 per rank). Passives in note
  "Build colors" (howto section 7).
- Icons: user sheet `new icons/Frodo type icons.png` -> `hacks/patches/skill_expansion/frodo_icons/`.
- Burden bar: HUD gap between HP and Spirit, notches at 51/91. Gold < 51, red 51..90, red/dark pulse
  at 91+, white flash on change, dark during Claim. Bank 0 has no violet, so Evil reads as red here.
Tested in game: HUD bar at 50/75/96, tree page 2 build colors and columns (v14). Untested: every skill effect.

## Frodo Burden rework (2026-10-21, ROM ROTK-Mod-Frodo-v15.gba)
Problem: stock Burden only rises from The One Ring and drains by itself, so Good was free.
Now (tickBurdenPressure + frodoIncomingDamage in skill_expansion_frodo.c, tuning defines at the top):
- Stock drain held off every tick (countdown refilled).
- Foe within 128 px = in combat: +1 Burden every 1.33s (+1/6 s per Strength of Will level), tripled after user feedback (v16). No fade.
- Out of combat: -1 every 3s. Verified in game: 30 -> 27 -> 24 over 2x10s.
- Nazgul-family hits: +3 Burden (Will refunds part).
- Paused while the stock Ring is worn or Claim It runs.
Untested: in-combat pull rate, Nazgul hits. Tuning knobs: PULL_BASE_FRAMES, FADE_FRAMES,
NAZGUL_HIT_BURDEN, PRESSURE_RADIUS.
Next: Put On the Ring as a 6th page 2 active (needs row 12, see howto "Page 2 slot limits").
Page 1 ties (v17, wraps in skillExpansion_frodoWrapStockCallbacks):
- Knife Toss at 51+: 2s shadow knife window, +10% ranged dmg (+20% at 91+), SPARK SHADOW on hit.
- Galadriel's Cloak: -15 Burden + SPARKLE_RING GHOST. Pull pauses while Frodo is hidden (ACTOR_FLAG_23,
  set by the Cloak and the Ring).
- The One Ring: unchanged +50, Strength of Will refunds part (resistBurden).
- Ring's Persuasion: +5 Burden per cast.
- Snare: stunned (ACTOR_STATE_STUNNED, assumed to be the Snare hold) and persuaded foes don't count
  toward the pull. Also covers Phial/Elbereth stuns.
Untested: all page 1 ties (save is level 1). Stock page 1 descriptions still show stock text.

## Aragorn v19 (2026-10-21, built, page 1 text verified on Frodo, Aragorn untested in game)
Columns: Ranger | Dead | King. Passive first levels rise in list order (tree sorts by first level).
- Row 1: Strider's Riposte 10 | Restless Dead 11 | King's Presence 12
- Row 2: Hunter's Patience 13 | The Dead Ride 14 | The Blade Reforged 15
- Row 3: Track the Quarry 17 | Lord of the Dead 20 | Hands of the King 21
Enum renames: TRACKER->HUNTERS_PATIENCE, RANGERS_REST->TRACK_THE_QUARRY, CAPTAIN->RESTLESS_DEAD,
MOMENTUM->THE_DEAD_RIDE, HEIR->BLADE_REFORGED, HEALERS_TOUCH->HANDS_OF_THE_KING (same ids, saves OK).
- Patience: 1.5s without a melee blow -> SPARK GREEN glint, next blow +20%/lvl, +10 Flame.
- Quarry: marked foe dies -> Mark jumps to nearest foe, +2s/lvl.
- Restless: owned ghost lifetime <= 3 -> ghostBurst (shared with Oath) 6+6/lvl, 1s stun, +5 Flame.
  Assumes ghost lifetime counts down 1/frame. Unverified.
- Dead Ride: kill chain (3s window, ghost kills count), every 8-lvl kills raises a ghost (cap 4).
- Blade Reforged: HP < 30% -> meter to 100 (or +4s if burning), cooldown 90s-10s/lvl.
- Hands: kills during Flame heal 2%/lvl max HP, partner too in co-op.
Page 1 ties (wrapStockCallbacks, Aragorn only): Sweep +8, King's Command +10, Numenorean Will
(5s/lvl: hurt doesn't damp), Call of the Dead (ghost hits +2), Herbal Healing +15. Projectile hits
(Sword Throw, also Oath/Restless bursts) stoke METER_HIT and scorch during Flame.
Flame bar is the HUD green ramp: Charge LG/G/DG, Bright W/LG/G (flash + blaze pulse), full pulses Glint W/LB/DG.
Aragorn icon sheet (`new icons/aragorn icons.png`) still not sliced.

## v20 (2026-10-21, ROM ROTK-Mod-Frodo-v20.gba)
- Aragorn icons: 14 icons sliced from the user's sheet into skill_expansion/aragorn_icons/ (5 page 2
  actives, 9 elite passives). Not seen in game yet (no Aragorn save).
- Row 12: optional 6th page 2 active, bottom slot. See howto-heroes-skills.md "Page 2 slot limits".
- Frodo Put On the Ring (row 12, Evil, level 15/18/21/24/25, 20 Spirit): wraps the stock One Ring at
  the row's level but undoes its +50 Burden, adds +5, and pins its timer so it lasts until Frodo attacks
  (stock 30-frame cap) or casts it again (free, refunds Spirit). Burden +1 every 15+3*level frames.
  Auto-removes at 100 Burden. Shadow of the Ring costs 0 while worn.
  Verified on the v12 save: tree slot, buy, cast, hold past stock duration, take off, attack reveal.
  Button flow verified too (A on, A off with refund). The stock One Ring says "Ability unavailable"
  while Put On the Ring is worn (stock refuses a second Ring), which is fine.

## v21 (2026-10-21, ROM ROTK-Mod-Frodo-v21.gba)
- First Aragorn in-game test (new game as Aragorn, gdb level/skill grants). Verified: page 2 icons and
  build tints, Flame bar green, Sweep +8 and King's Command +10 stoke, Flame of the West full-bar cast,
  Return of the King (free costs, then restored), Oath with no ghosts.
- Bug fixed: all 5 Aragorn page 2 actives inherited the stock buff bit of their baseRow, so Flame of
  the West, Oath and Return were uncastable during Numenorean Will. Now use pose-only policies.
- Not tested (no enemies at mission start): kill/hit stoking, Ambush strike, Tracker's Mark, ghost hits.

## Smeagol page 2 "Gollum's Grip" as built (2026-10-21, ROM ROTK-Mod-Smeagol-v22.gba, untested in game)

Code: hacks/patches/skill_expansion/skill_expansion_smeagol.c (Frodo pattern, no kit), data in content.c, passive ids PASSIVE_SKILL_COUNT+36..44.
Grip 0..100 per player, no HUD bar. Shown by palette tint (hackLib_fxTintSlot on ObjPalettes[0x21+HERO_ID_SMEAGOL]), green sparks at 75+, feedback barks at 25/50/75/90.
- Gain: +1 per 18f with a foe within 112px (Poor Smeagol slows it), +2 per hit taken, +3 per kill. Decay -1 per 45f out of combat.
- At 100: takeover 120f (+60 We Wants It). input_update wrapper steers to the nearest foe and presses B. A presses cut 6f each. Damage x2, skills locked. Ends at grip 40.
- Columns: Smeagol (GOOD) | Gollum (EVIL) | Precious (TRICKS). Levels 10..25.
- Actives: Play Dead (Cower pose), Throttle (ride and steer a stunned foe), Drop the Precious (bait lure, grab for burst, or forced takeover), Catch a Fish (catch type 0x37 projectiles, throw back x2), Slink (hop forward through walls, hide), Argue With Yourself (lower grip, stun at 60+).
- Passives: Poor Smeagol, Hungry, Sniff It Out, Nice Master, Rat Catcher, Finders Keepers, Smeagol Is Free, We Wants It, My Precious!.
- My Precious! deviates from the pitch: an extra loot_rollDrop on kills at 70+ grip, not forced prefix gear.
Test list: tint visible, takeover steering plus A mash, Play Dead hiding (ACTOR_FLAG_23), Throttle steer and wall checks, Catch a Fish on archers, Slink skipping gates (possible sequence break), bait pull, Sniff It Out sparkle.
- v23 (ROTK-Mod-Smeagol-v23.gba): Catch a Fish crashed in v22. It no longer spawns a projectile (actor_projectile_create from a non-shooter hero was the suspect, untraced). The catch now marks the arrow PENDING_REMOVE and deals the damage straight to the shooter. Grip no longer tints Smeagol. Only the takeover recolors him, fading over 16 frames to a hand-made GollumColors palette (desaturated grey-green, crushed shadows), with shadow smoke puffs.
- v24: Drop the Precious was grabbed back instantly (Ring lands under Smeagol). Pickup now arms only after Smeagol spends BAIT_ARM_FRAMES beyond BAIT_LEAVE_RADIUS (32px) of it.
- v25: Catch a Fish still crashed ("jumped to invalid address"). Suspect: hackLib_fxTimed on FX_SHAPE_OVAL_RING (anchor flag 0x20002, never proven with the timed loop). Now a timed SPARKLE_RING aura (proven) + one-shot OVAL_RING castFx, catch spark spawned with fxAt instead of attached to the dying arrow. Unconfirmed in game.
- v26: Smeagol reverted to an 'ugly old' palette after takeover. Cause: his real palette is ObjPalettes[0x21 + 9] (applyTypeDefaults remaps hero 7 to 9). GollumColors regenerated from it.

## Sam page 2 "Samwise's Garden" as built (2026-10-21, ROM ROTK-Mod-Sam-v27.gba, untested in game)

Code: `hacks/patches/skill_expansion/skill_expansion_sam.c`, data in skill_expansion_content.c, hooks in elite.c (tick, reset, cleanup, samDamage melee + ranged, samIncomingDamage with origin). State lives in the shared hero block (see hacking-recipes.md MODCODE_BSS notes).

- Plants are position-only (pixel x/y), drawn with one-shot `hackLib_fxAt` every 20 frames: taters SPARK -> SPARKLE_RING green -> STAR_BLOOM gold, brambles SPARK -> TENDRIL_BURST green -> TENDRIL_BURST brown. 3 slots (+1 Master Gardener). A full garden replaces the ripe plant nearest wilting, else the most grown one.
- Growth: ripe after 6s, x2 within 40px of Sam, Green Thumb +20%/level. Ripe lasts 8s (+2s/level Gaffer's Lore), then wilts.
- Seeds land 20px in front of Sam so he does not harvest at once. Walking within 12px of a ripe tater harvests it (6 +2/level Spirit, Lembas heal, Fine Seasoning Spirit).
- Columns Gardener (GOOD) | Stout (EVIL) | Cook (TRICKS). Rows 7..12: Plant Taters, Elven Rope, Second Breakfast, Bramble Hedge, Frying Pan Parry, Po-tay-toes!.
- Base rows: plants use Snare pose (SnarePosePolicy, RULE_NONE skips the snare count limit), Second Breakfast / Parry use Cookpot pose (CookpotPosePolicy), Rope / Po-tay-toes use Knife Toss (needs a main hand weapon, stock rule).
- Ripe taters pelt foes within 40px every 2s (6 +3/level, 1s stun). Brambles snag every 1s, radius 20/30/40 by stage (3 +2/level, 10 frame stun).
- Stew: 8s +1s/level at Sam's feet, heals players within 40px 2% +1%/level max HP per second. Taters for All: +20% damage near it and 1.5s after.
- Parry: blocks all damage 2s +0.2s/level, each block is a charge (max 5), end bonk 36px for (6 +3/level) x charges + 1s stun. Shire Grit: +3/level per block to the next melee hit (cap 99).
- I Can Carry You: once per area (cleanup clears it), a hit that leaves Sam at <= 25% HP is cancelled, heals 40%, 2s immunity. Self only.
- Icons are stock Sam icons for now (Snare used twice). Candidate for user art ([[custom-skill-icons]] pipeline).

## Saruman "The Prism" as built (2026-10-21, ROM ROTK-Mod-Saruman-v28.gba, untested in game)
Code: `hacks/patches/ringwraith/saruman.c` (kit), passives + build tables in `skill_expansion_content.c`,
ids `SKILL_EXPANSION_PASSIVE_LORE_OF_COLOURS..PARTING_SHOT` (PASSIVE_SKILL_COUNT + 54..62, END +63).
Host test `hacks/tests/saruman.c`. RAM: 0 new .bss, `SarumanState` (32 B) lives in
`skillExpansion_heroState[p]`, every access gated on heroId 9.

Columns (internal build enum only picks colors): GOOD = Curunir, EVIL = White Hand, TRICKS = Sharkey.

Prism: each successful spell adds its color (White / Isengard / Voice). Same color twice in a row
adds nothing. All three held = refraction, decided by the color that completed it. Prism fades
after 6s without a new color. After a refraction the next spell costs half (free with Head of the
White Council). Costs are rewritten every tick from ROM costs, so reset/cleanup/death/level up
restore them.
- White: heal 8% max HP, 25% damage reduction 6s.  Isengard: 64px nova 20 + 2/hero level.
  Voice: 96px stun 4s. Lore of Colours +15%/lvl to all of it.

Page 1 (stats: strength 16 -> 10, courage 32 -> 36, melee damage halved, +1 Spirit / 30 frames
on top of stock ~6/s):
| row | name | color | effect |
|0|Voice of Saruman|Voice|64px, 4+3/lvl dmg, stun 45+15/lvl f|
|1|Flame of Orthanc|Isengard|10+7/lvl hit + Burn 2+lvl/s for 150+30/lvl f|
|2|Blasting Fire (replaces Wizard's Force)|Isengard|target in 128px, 40px burst 28+8/lvl, stun 30 f|
|3|Many-Coloured Ward|White|20+5%/lvl DR for 180+60/lvl f|
|4|Word of Binding (replaces Servant)|Voice|6+3/lvl dmg, stun 120+30/lvl f, spells +25% vs it|
|5|Orthanc's Remedy|White|heal 12+8/lvl|

Page 2 (pyramid Curunir | White Hand | Sharkey, Curunir | Sharkey, White Hand):
| row | id | name | column | cost/cd | effect |
|7|0xa10|Echo of Power|Curunir|30/45f|recast last page-1 or Furnace/Curse/Spite at 40+10/lvl % power, adds White|
|8|0xa09|Furnaces of Isengard|White Hand|50/240f|up to 5 foes in 72px Burn 3+2/lvl/s 6s|
|9|0xa11|Sharkey's Curse|Sharkey|25/120f|5+3/lvl hit, curse 480+60/lvl f: spells +20+5/lvl %, each hit leaks 2 Spirit|
|10|0xa12|Light Broken|Curunir|30/120f|needs Prism: 80px (18+8/lvl) x colors held, +8 Spirit/color, empties Prism|
|11|0xa13|Sharkey's Spite|Sharkey|35/120f|72px (16+6/lvl) x (100 + missing HP%)%, steal 2 Spirit per foe|
|12|0xa0c|Power of the White Hand|White Hand|90/360f, lv 30..38|104px 45+15/lvl stun 60f, then refract as last color (Isengard if none)|

Passives (first level): Curunir Lore of Colours 20, Ring-lore 24 (4 Spirit/lvl per refraction),
Head of the White Council 28 (post-refraction spell free). White Hand Fires of Industry 21 (Burn
+1/s and +2s per lvl), Machine of War 25 (spell kill +2 Spirit/lvl), Powder of Orthanc 29 (spell
kill explodes 40px for 20 + hero level, SECONDARY so no chains). Sharkey Cutpurse 22 (cursed foe
dies: steal 5 Spirit/lvl and curse jumps 96px), Spite 26 (below 40% HP spells +6%/lvl), Parting
Shot 30 (dropping below 30% HP fires a free level-matched Spite blast and curses nearest foe, 20s cd).

Risks: cost rewrite every tick (any other writer of Saruman's RAM costs gets clobbered, none known);
Parting Shot fires from tick outside a cast damage source (burst sets its own kind/row); Word of
Binding stun then a hit could break the hold on some enemy types; old save slots keep page 1 ids
0xa01..0xa06 so Wizard's Force/Servant points carry into Blasting Fire/Binding; old page-2 ids
0xa08/0xa0a/0xa0b vanish (points lost), 0xa09/0xa0c keep their points.
Test list: refraction of each color + feedback text; half / free cost shows in tree and HUD and
reverts after the next cast, on death and on level up; Echo with nothing cast (should refuse);
Light Broken with empty Prism (refuse); curse puff + jump on kill with Cutpurse; Powder chains
not looping; Parting Shot once per 20s; co-op with Frodo/Sam/Smeagol (shared heroState untouched).

### Saruman page 1 passives (DRAFT v1, not built)

Replace the 9 vanilla page 1 passives. Three per color, no overlap with page 2. First ranks at levels 1..9, +2 levels per rank.
| # | name | color | effect |
|1|Robes of Many Colours|White|3%/lvl less damage taken while the Prism holds a color|
|2|Keeper of Orthanc|White|Remedy heals +10%/lvl, overheal becomes Spirit|
|3|Wisdom of the Istari|White|a new Prism color refunds 1 Spirit/lvl|
|4|Kindling|Isengard|Blasting Fire +10%/lvl vs burning foes|
|5|Wildfire|Isengard|hit on a burning foe: 10%/lvl chance burn spreads to nearest foe in 48px|
|6|Staff of Orthanc|Isengard|melee hits burn 1s and refund 1 Spirit/lvl|
|7|Honeyed Words|Voice|stuns and holds +10%/lvl duration|
|8|Voice of Command|Voice|stunned/held foes take +5%/lvl from Isengard spells|
|9|Lies of Saruman|Voice|stunned foes deal 5%/lvl less to you for 4s after|
Open: vanilla stat grants lost (fold into base stats, or keep per passive). New ids append before SKILL_EXPANSION_PASSIVE_END.

### Saruman visuals v29 (ROTK-Mod-Saruman-v29.gba, untested)

Every cast rebuilt as 3 to 6 paced layers plus sfx, new `FX_PALETTE_VENOM`. Full map in fx-catalog.md "Saruman map". The old casts used SHOCKWAVE EMBER/GREEN, which draw grey and black.

### Saruman levels v31 (ROTK-Mod-Saruman-v31.gba, untested)

Page 2 moved to the Frodo band. Actives: Echo 10, Furnace 11, Curse 12, Light Broken 13, Spite 14, White Hand 15 (all max by 25). Passives: Lore of Colours 10, Fires 11, Cutpurse 12, Ring Lore 14, Machine of War 15, Spite 16, capstones Head of the Council 22, Powder 23, Parting Shot 24. Was 20 to 38.

### Saruman visuals v30 (ROTK-Mod-Saruman-v30.gba, untested)

Each spell has a unique signature shape and plays as cast -> bolt travel -> cued impact via the
`cue`/`launchBolt` sequencer in saruman.c. Map and sequencer notes in fx-catalog.md "Saruman map (v30)".
Host tests widen SKILL_EXPANSION_HERO_STATE_WORDS to 32 because 64-bit pointers grow SarumanState.

### Sam visuals (ROTK-Mod-Sam-v28.gba, untested)

All garden props and hit markers are PNG sprites (15 sheets, shared palette slot), drawn by hacks/tools/sam_fx_art.py. Map in fx-catalog.md "Sam map".

## Gimli page 2 "The Tally" as built (2026-10-21, ROM ROTK-Mod-Gimli-v31.gba, untested in game)
Code: skill_expansion_gimli.c/.h, content in skill_expansion_content.c, tree purchase in skill_expansion_tree.c.
Rule (Gimli only): page 2 costs no skill points. It is bought with kills.
- Tally = stock `Player.killCount` (u16, already in the stock save, so old saves keep their count). Stock `player_awardXp` adds 1 per enemy death for every active player, so co-op partner kills count too.
- Marks = killCount / 10 minus page 2 ranks bought. One mark buys one rank.
- Page 2 requiredLevels for Gimli are in steps of 20 kills (`GIMLI_TALLY_KILLS_PER_STEP`). The tree shows "(Kills N)" with N = step x 20, and "Maxed" when done.
- Tree: `hookReplace scene_skillTree_handleA, skillExpansion_treeHandleA`. On a Tally page it calls `buyWithTally`, else stock `scene_skillTree_buySkill`. The points box shows marks and the title reads "Tally: N kills".
- Bonus Tally (`skillExpansion_gimliAddTally`) adds to killCount, pays the stock +2 attribute points per 500 kills on crossing, and skips per-kill passives for the bonus kills.
- Charge bar (v35, HUD gap meter, gold with a notch at 5, always on for Gimli): every Tally kill (party kills too) adds 1 of 10. Full: pulses white, "Next skill empowered!". The next successful active cast (any row, stock or page 2) is free (paidSpirit refunded) and Gimli deals +50% damage from cast start until 5s after it ("Empowered!", gold ring). A failed cast keeps the charge. Hooks: skillExpansion_gimliCastStarted in skillExpansion_cast, skillExpansion_gimliCastEnded at the top of skillExpansion_finishCast. Resets on area change. Marks pop "Tally mark!" only when the charge pop does not.
Columns: Stone (GOOD gold) | Fury (EVIL) | Tally (TRICKS). Rows 7..12 = Baruk, Cleave, Toss / Hold, Grudges / Still Counting.
Actives (required steps x20 kills):
- Baruk Khazad! (Stone, 35 Sp, 1/3/5/8/12): stun foes within 80px 1s +0.2s/level. For 4s +1s/level take 4% less damage per foe stunned (cap 40%).
- Cleave the Weak (Fury, 30 Sp, 4s cooldown, 1/3/6/9/13): nearest foe within 40px. Below 15% +3%/level HP and with HP <= Gimli's max HP it is executed (0x7fff damage), the Spirit paid is refunded and the cooldown is cleared next tick. Else 8 +4/level.
- Nobody Tosses a Dwarf (Tally, 40 Sp, 2/4/7/10/14): leap to 14px short of the nearest foe within 112px (findLanding), 40px slam for 10 +5/level, 1s stun on survivors, each landing kill +1 bonus Tally.
- Hold the Deep (Stone, 45 Sp, 4/7/10/14/18): rooted 3s (coords pinned each tick), takes half damage, no flinch. Stores full damage taken (cap 400). Ends in a 56px quake for 10 +4/level + stored x (60 +10/level)%.
- Book of Grudges (Tally, 20 Sp, 5/8/11/15/20): nearest foe within 128px. Its unitType is the grudge until area reset: +10%/level damage to that type, and their kills give +1 bonus Tally.
- Still Counting (Fury, 80 Sp, 15/22/30/40/50): 12s +2s/level. Each Tally kill heals 3% max HP, adds 1s and shows the count. Ends in a 72px burst of 4 per kill (cap 300).
Passives (first required step x20 kills):
- Tier 1: Dwarven Mail (Stone, 1): -2%/level damage per foe within 48px, up to 4. Grudgebearer (Fury, 2): +4%/level more vs the Book of Grudges type (grudge hits show a blood spark). Friendly Wager (Tally, 3): 3 own kills within 5s gives +5%/level damage for 8s ("Beat that, elf!", gold ring).
- Tier 2: Not Without Me (Stone, 6): co-op, partner within 96px, Gimli takes 8%/level of the partner's hits (never below 1 HP). Blood on the Axe (Fury, 7): kills while raging add 0.5s/level to the Rage buff and aura (cap 30s left). Orc-Count (Tally, 8): each mark restores 4/level Spirit.
- Capstones: Stubborn as Stone (Stone, 19): never flinch. Durin's Folk Don't Fall (Fury, 20): once per Rage a killing blow leaves 1 HP. Forty-Two! (Tally, 21): every 42nd kill a free 64px quake (15 +5 x Earth Shatter level, 1s stun) and full Spirit.
Clarity pass (v33): Forty-Two pops "Forty-two in 2/1...", Durin's Folk pops "Durin's ward is up!" with a gold bloom when Rage starts, Blood on the Axe flares a blood spark per extension.
Deviations from the draft: Baruk is a stun war cry, not a taunt (no taunt hook yet). No boss bonus. Grudgebearer uses most-slain family this area, not 50 lifetime kills. Hold the Deep also blocks flinch. Numbers retuned (Not Without Me 8%/level, Orc-Count 4/level Spirit, Dwarven Mail cap 4, Blood on the Axe 0.5s/level).
Icons: user art from `new icons/Gimli-Legolas-Gandalf.png` (all 15 page 2 skills, actives and elite passives via `.icon`, v3).
Save note: Gimli's page 2 slots used to hold stock placeholder passives. Any ranks bought there on old saves now count against marks for the new skills.
Test list (v32: Debug page "Max Kills" sets the Tally to 1000 = 100 marks, enough for all 63 ranks):
1. Gimli tree page 2: title "Tally: N kills", points box = marks, descriptions show "(Kills 20)" etc. A buys only with a mark and enough kills (error sfx otherwise). Page 1 still uses skill points. Other heroes unchanged.
2. Kill 10 foes: HUD bar fills and flashes, one mark more.
3. Each active: Baruk stuns + guard ring, Cleave execute refund + instant recast, Toss leap/slam (no landing = "No safe landing"), Hold root/quake, Grudges bonus, Still Counting heal/count/bar/burst.
4. Passives: Mail, Grudgebearer, Wager, Blood on the Axe (Rage lasts longer), Orc-Count Spirit, Stubborn no flinch, Durin's 1 HP save, Forty-Two quake at kill 42/84...
5. Co-op: Not Without Me share, Wager uses partner kills, tree page can't turn (stock rule).

## Gandalf page 2 "The Grey and the White" (BUILT v36 2026-10-21, untested in game)
Code: hacks/patches/skill_expansion/skill_expansion_gandalf.c (all logic), content.c (defs, passives, builds),
hooks in elite.c (incoming damage, tick, reset, cleanup, melee/ranged damage, onKill) and smeagol.c inputUpdate.

### As built (differences from the draft below)
- Builds: Grey Pilgrim = new BUILD_GREY ramp, White Rider = GOOD gold, Narya = new BUILD_FIRE ember ramp.
  Page 2 active icons take GREY or GOOD by the active player's current form (activeBuild branch).
- Icons: user art from `new icons/Gimli-Legolas-Gandalf.png` (all 15 page 2 skills, actives and elite passives via `.icon`, v3). One icon per row, not per form.
- Costs: rewritten every tick from GreyCosts/WhiteCosts (skipped while the tree shows page 2). Old Tricks discounts
  Grey halves, Quickened discounts every row until the next cast starts. Tree text "@2" shows the current form's cost.
- No-regen White: STAT_SPIRIT_REGEN and STAT_SPIRIT_REGEN_PERCENT are zeroed and saved, re-zeroed if the game rebuilds
  them, restored on Grey/reset/cleanup. Drain 3 Spirit/s, Unbending -0.4/s per level, paused 10s after the Fall return.
- Wandering Wizard: +1 Spirit every 3s -0.4s/level while Grey (not a regen percent).
- Smoke Rings: a static 40px cloud 64px ahead for 4s, stuns 0.5s on cast. White spells on foes inside +10%/level.
- Breaking of the Staff: 80px zone at Gandalf for 3s +0.5s/level. Any hit on foes inside +25%, foes inside deal half.
- Fly, You Fools!: +50% speed for Gandalf only (partner skipped, avoids co-op stat desync).
- Swap hint: once per area until the player swaps once since power on (static flag, no save bit).
- Death clears form, zones and timers (keeps The Fall's once-per-area flag).

### Test list (v36)
1. Below level 10 hold R does nothing. At 10+ "Hold R: Gandalf the White" pops at area start until the first swap.
2. Short R still opens chests and loot. L+R quick slot still works. Hold R 0.5s swaps. Under 20 Spirit: "Not enough Spirit".
3. White: robe fades to white, sparkles every 2s, Spirit bar fill turns white and drains 3/s with no regen.
   Under 25 it pulses and pops "The light dims". At 0 "The light fades" and back to Grey, bar back to blue.
4. Return stuns foes in 64px. Smoke hides Gandalf 2s (foes lose him). Swap cooldown 3s.
5. Tree page 2 shows grey icon ramp in Grey, gold in White. Costs in the text change with form. Page flip and
   back leaves page 1 costs intact.
6. Each row both halves: Fireworks burst + 3 colored pops / Flame of Anor beam. Never Late snaps back after 4 pulses
   (restores higher HP/Spirit, "The way back is shut" when blocked) / Shadowfax charge stops at walls, pushes foes.
   Smoke cloud stuns / Broken Staff zone. Pipe 3s hidden +Spirit, any input stops / Theoden heals self + partner.
   Fly shove + haste ends after 3s / Look to My Coming 160px stun. YSNP line pushes foes 3s, burst, fall, returns
   White with full Spirit / Valinor spends all Spirit, damage scales with it, back to Grey.
7. The Fall saves a lethal hit once per area in Grey. Robes, White Rider, Secret Fire nova on every swap.
8. Save/load mid-White: regen comes back (stats rebuild), form resets to Grey.
9. Risks: stock regen might not read STAT_SPIRIT_REGEN (then White still regens). Hold R may clash with something unseen.
   Stock spells must get +30% in White (they hit with no damage source).

### Draft v2 (design reference)
Identity: the only hero who changes form at will. One rule drives everything: **Grey saves Spirit, White spends it.**
No new meter (the Spirit bar is the timer). Avoids: bar meters (Frodo, Aragorn, Smeagol), color combos (Saruman),
placed objects (Sam), kill currency (Gimli), thralls (Ringwraith), Burn (Saruman's White Hand).
Levels 10..25. Columns = the three ways to play the form: Grey Pilgrim (TRICKS, stay Grey) | White Rider (GOOD, stay White) | Narya (EVIL color, switch often).

### Form rules
- Swap: **hold R for 15 frames** (about 0.5s). It doesn't use a skill slot, and the swap works the same in cycle mode and quick-cast mode.
  A short R press still opens chests and picks up loot (stock acts on the press, the swap needs the hold). Hook: input_update wrapper (Smeagol precedent).
  Blocked while casting, in overlays and during the Fall. Cooldown 3s. Grey to White also needs 20+ Spirit ("Not enough Spirit" popup).
- Unlock: level 10 (first page 2 level). Until the player swaps once, every area start pops "Hold R: Gandalf the White" (one mod_save bit).
- Grey: stock Spirit regen. Page 2 slots cast their Grey half (cheap, 10..20 Sp).
- White: **no Spirit regen, -3 Spirit/s**, page 1 spells +30% damage/effect, page 2 slots cast their White half (35..60 Sp).
  At 0 Spirit: "The light fades", back to Grey (no smoke payoff, 3s cooldown).
- Swap payoffs (base, every swap):
  - Grey to White, "Return": 64px white flash, foes stunned 1s.
  - White to Grey, "Smoke": puff at Gandalf's feet, foes within 64px lose their target for 2s (ACTOR_FLAG_23 for 60 frames).
- Resets: area start, death and cutscenes put Gandalf back in Grey.

### Clarity / HUD
- Sprite: White = hand-made white robe palette (GollumColors method, ObjPalettes[0x21 + HERO_ID_GANDALF]), 16 frame fade, faint sparkles every 2s.
- Spirit bar: while White, HUD bank 0 colors 10..12 (Spirit fill only) become white/silver, so the draining bar IS the White timer.
  Under 25 Spirit it pulses and pops "The light dims" once.
- Skill icon panel: shows the half for the current form (12 page 2 active icons, user art). The ramp is grey in Grey, gold in White, and magenta when uncastable (stock rule).
- Tree text: each page 2 active reads "Grey: ... / White: ...", with both costs.
- Popups: "Gandalf the White!", "The Grey Pilgrim", "The light fades", plus one per active result listed below.

### Actives (one role per slot. The Grey half is the cheap version, the White half the big one)
| row | role | Grey half | White half |
|7|Damage|Fireworks, 15 Sp: rocket flies 64px and bursts (40px, 8 +4/lvl, 0.5s stun) into 3 pops (4 +2/lvl)|Flame of Anor, 45 Sp: white fire beam across the screen in the facing direction, pierces, 20 +8/lvl|
|8|Movement|A Wizard Is Never Late, 15 Sp: staff mark on the ground (pulses each second, 4 pulses). Then Gandalf snaps back to it, and HP and Spirit go back to the cast values if those were higher. Undoes a bad fight.|Shadowfax, 40 Sp: charge up to 160px along the facing line, stopping at walls (findLanding). Every foe on the path takes 14 +6/lvl and is knocked aside.|
|9|Control|Smoke Rings, 10 Sp: a slow ring drifts 96px. Foes it touches are slowed 3s and take +10%/lvl from the White half spells. This is the Grey to White setup.|The Breaking of the Staff, 40 Sp: foes within 80px are "broken" 3s +0.5s/lvl: they deal half damage and take +25%. A grey spark marks them.|
|10|Sustain|Pipe-weed Pause, 0 Sp: channel up to 3s (any input cancels). +8 Spirit/s +2/lvl, smoke hides Gandalf (ACTOR_FLAG_23).|Healing of Theoden, 40 Sp: heal 20% +4%/lvl max HP. In co-op the partner within 96px gets the same. White has no regen, so this is its only sustain.|
|11|Crowd|Fly, You Fools!, 20 Sp: foes within 80px are knocked out of the ring. Gandalf (and the partner) get +50% speed 3s. This is the escape.|Look to My Coming, 60 Sp: sunrise over the whole screen, every foe takes 16 +6/lvl and flees 3s.|
|12|Ultimate|You Shall Not Pass!, 70 Sp (lv 18..25): a staff line in front blocks foes 3s (any that cross are pushed back). Then the bridge breaks: 96px burst 30 +10/lvl. Gandalf falls (hidden and invulnerable 90f) and returns White with full Spirit, drain paused 10s.|Light of Valinor, all Spirit (min 30): screen burst, damage = Spirit spent x (40 +10/lvl)%, then back to Grey (Smoke payoff fires).|
Purpose check: Grey has damage, escape, undo, setup and Spirit gain. White has burst, control, mobility, heal and finisher.
The ultimate pair is the loop: You Shall Not Pass is the big way into White, Light of Valinor the big way out.

### Passives (column = playstyle, row = tier)
| tier | Grey Pilgrim (stay Grey) | White Rider (stay White) | Narya (switch often) |
|1|Wandering Wizard: Grey Spirit regen +15%/lvl|Robes of White: White takes -5%/lvl damage|Kindler of Hearts: swap payoff radius +12px/lvl, stun/lose-target +0.2s/lvl|
|2|Old Tricks: Grey halves cost -10%/lvl|Unbending: White drain -0.4/s per lvl (3 to 1 at lvl 5)|Quickened: swap cooldown -0.5s/lvl. The first spell after a swap costs -10%/lvl.|
|3|The Fall: once per area, a lethal hit in Grey triggers You Shall Not Pass's fall instead (pops "Fly, you fools..." then "I have returned")|White Rider: White kills restore 3 Spirit/lvl (fighting keeps White alive)|Servant of the Secret Fire: every swap also sends out a 56px fire nova, 10 + hero level|

### Tests (when built)
Hold R swaps, a short R still loots. The Spirit bar turns white and drains, then turns Grey at 0. Palette fades both ways.
Icons change per form. Each half casts the right effect. Never Late restores and refuses walls. YSNP line holds foes.
The Fall works once per area. Co-op: Theoden and Fly You Fools affect the partner, and each player swaps on their own R.

### To RE / risks
- What R does besides loot (if R also dodges or blocks, use hold L+R instead). Check the asm input handlers.
- Gandalf's real ObjPalettes slot (Smeagol was remapped). Spirit HUD colors 10..12 must be Spirit-only (doc says yes).
- Enemy pushback for the YSNP line and knockback (write foe coords, wall check). Beam hit test along a line (iterate on-screen foes, distance to the ray).
- Twin halves on one row: the cast callback branches on form, and the HUD icon swaps through icons[row] (both icon sets in ROM).
- State lives in the shared skillExpansion_heroState block, gated on HERO_ID_GANDALF.

## Legolas page 2 "The Glint" (BUILT v1 2026-10-23, ROM ROTK-Mod-Legolas-v1.gba, untested in game)
Code: hacks/patches/skill_expansion/skill_expansion_legolas.c (all logic), content.c (defs, passives, builds),
hooks in elite.c (incoming damage, flinch, tick, reset, cleanup, ranged damage), smeagol.c inputUpdate (loose
detection), runtime.c rejectCast ("Needs a bow"), skill_expansion.asm (hookReplace player_tickOverdrawCharge).

### As built (differences from the draft below)
- Glint timing: the draft said "30 frames of hold". Built as GLINT_FRAMES = 12 frames after the draw reaches full
  (state 0x60 with previousAnimationFrame == 9, the same test the stock body uses), since the stock shot only fires
  on a B release at full draw. True window = frames 12..17 at full draw (+1 per Elf-Sight level).
- A release before full draw is not a loose: no shot, streak unchanged. Plain B taps do not touch the streak.
- Keen Eyes is renamed Elf-Sight (stock Legolas already has a Keen Eyes passive).
- Unique bows: Overdraw now starts with any bow family item (stock BUGFIX logic in the replacement). Risk: the stock
  firing path may still check ITEM_TYPE_BOW, so a unique bow may draw but not shoot.
- True Shot bonus: projectile hits (HACKLIB_DAMAGE_PROJECTILE) within 45 frames of the True loose get
  +25% +5%/Steady Hand level per streak step. Ability hits are never boosted.
- Shield-Surf normal slide is 48px, True is 96px plus 1s untouchable (incoming damage 0) and hidden (ACTOR_FLAG_23).
- Gift True refunds the whole nock cost (Fletcher does not stack on it). Twin Nock skips the ultimate (it already
  spent the streak).
- Point Blank and Elven Cloak trigger on every True Shot, nocked or not.
- Death clears the nocked arrow without a refund, plus the streak and any pending Volley.
- Builds: Eye = GOOD gold, Quiver = TRICKS green, Grace = DEAD pale teal (no new ramp). All page 2 active icons use
  TRICKS, the nocked row shows GOOD until loosed.
- Icons: user art from `new icons/Gimli-Legolas-Gandalf.png` (all 15 page 2 skills, actives and elite passives via `.icon`, v3).

### v2 (ROTK-Mod-Legolas-v2.gba, 2026-10-23, untested)
User feedback on v1: no visible glint cue, and hold-to-charge registered inconsistently (sometimes one arrow).
- Charge fix: stock counts bowHoldFrames only in two states (calls at 0x08023d2e idle, 0x080243c2 mid-shot)
  and never resets it on release, so taps add up and holds can miss. Now input counts a continuous B hold every
  frame (LegolasState.holdFrames, reset on release) and the charge replacement starts Overdraw at 15 frames of it.
  Applies at every level. Holding B still fires one tap arrow first (stock), then draws.
- Glint cue: SPARKLE_RING WHITE at full draw, then at the glint TENDRIL_STAR (streak color) + sound and the whole
  Legolas sprite glows toward the streak color for the full True window, fading out after. "True Shot!" /
  "True Shot xN" popup on every True. "Too early" / "Too late" while learning (no True Shot yet since power on)
  or when a streak or nocked arrow is at stake. "Release B on the flash" hint once per area until learned.
- Still level 10+ only for the glint.

### Test list (v1)
1. Below level 10 Overdraw behaves as stock. At 10+: hold B with a bow, full draw, then a colored SPARK + gem sound
   (the glint) about 0.4s later. Release in the next 6 frames: gold STAR_BLOOM + crit sound (True). Early/late: none.
2. Streak: glint white, then green after 1-2 True, gold 3-4, blue 5 with "Unerring!" once. A late/early loose resets.
3. True arrows hit visibly harder for 1.5s after the loose. Overdraw with a unique bow (equip one): does it draw AND
   fire?
4. Each row nock: Spirit paid, "<name> nocked" popup, row icon turns gold. Nock another: old cost refunded.
   No bow: "Needs a bow", no Spirit spent.
5. Loose each arrow normal and True: Piercing line hits all on the line (True +50%). Surf slides back, stops at walls,
   True = untouchable 1s. Pinning stuns first foe (True: neighbours too). Gift Spirit per foe (True refund).
   Volley 3 waves (True 5) 1s after the loose, 96px ahead. Only Counts as One scales with streak and resets it.
6. Passives: Swift Nock lowers tree costs (page 1 costs unchanged after a page flip). Elven Cloak, Fletcher refund,
   Point Blank shove, Unerring free line at streak 5, Twin Nock second half arrow, Unshaken: hits while drawing do
   not break the draw.
7. Co-op: each Legolas has his own glint and streak. Death mid-nock: nock gone, streak 0.
8. Risk: the loose is read in input_update before the state tick. If the stock body leaves state 0x60 a frame early,
   no loose is ever detected (glint shows, nothing fires). Fix by checking the release edge in the tick instead.

### Draft v1 (design reference)
Identity: the only hero whose system is a timing skill. One rule drives everything:
**draw the bow, loose on the glint.** No meter, no new button. Builds on the stock Overdraw (hold B with a bow).
Avoids: bar meters (Frodo, Aragorn, Smeagol), color combos (Saruman), placed objects (Sam), kill currency (Gimli),
thralls (Ringwraith), form swap (Gandalf), cheat-death passives (Gandalf, Gimli).
Levels 10..25. Columns: Eye (gold, the timing itself) | Quiver (green, nocked arrows) | Grace (teal, fighting up close).

### Core rules
- Glint: hold B. After 30 frames of hold a glint flashes on Legolas (SPARK + soft sound). Loosing (releasing B) in the
  next 6 frames is a **True Shot**. Earlier or later is a normal shot. Missing never costs anything.
  Unlock: level 10. Stock shots below 10 are unchanged.
- True Shot: the stock arrow's next hit within 45 frames does +25%, and a gold STAR_BLOOM shows on Legolas.
- Steady streak: back-to-back True Shots count 1..5, and a normal shot resets it. The glint color shows the streak
  (white 0, green 1-2, gold 3-4, blue 5). At 5: "Unerring!" once.
- Nocking: every page 2 active is an arrow, the ultimate too. Casting it nocks it (pays Spirit,
  popup "<name> nocked"). The next loose fires it: True = full effect, normal = half. One arrow on the string:
  nocking another swaps it and refunds the old one. Needs a bow ("Needs a bow").
- Effects resolve at the loose with their own hit tests (a line along the facing, like Flame of Anor), so they never
  depend on the stock arrow projectile.

### Clarity / HUD
- The glint is the HUD: its timing is the cue, its color is the streak. Sound on True (CRIT_HIT), none on normal.
- A nocked arrow: the HUD skill icon of that row uses the gold ramp until loosed. Popups only on nock, Unerring, swap.
- Tree text: "Nock: ... True: ..." on each active.

### Actives (same role grid as Gandalf: damage, movement, control, sustain, crowd, ultimate)
| row | role | nock (normal loose = half) | True loose |
|7|Damage|Piercing Shot, 20 Sp (lv 10): a line 240px through every foe, 14 +6/lvl|+50% damage|
|8|Movement|Shield-Surf, 15 Sp (lv 11): on the loose Legolas slides 96px backwards (wall-safe)|the slide is untouchable and hides him 1s|
|9|Control|Pinning Shot, 20 Sp (lv 12): first foe on the line is pinned (stunned) 1.5s +0.3s/lvl|also pins every foe within 40px of it|
|10|Sustain|Gift of the Galadhrim, 15 Sp (lv 13): +4 +2/lvl Spirit per foe on the line|refunds the nock cost too|
|11|Crowd|Volley of Helm's Deep, 35 Sp (lv 14): fired upward. 1s later 3 waves rain on a 64px circle 96px ahead, 6 +3/lvl each|5 waves|
|12|Ultimate|That Still Only Counts as One!, 50 Sp (lv 18..25): one huge arrow, first foe on the line, 30 +10/lvl. It spends the streak: +50% per streak step (x3.5 at 5)|counts the shot itself in the streak (a perfect 5 streak plus True = x4)|
Loop: True Shots build the streak, nocked arrows spend Spirit for effects, the ultimate cashes the streak.

### Passives (column = build, row = tier)
| tier | Eye (gold, timing) | Quiver (green, nocked arrows) | Grace (teal, up close) |
|1 (10/11/12)|Keen Eyes: glint window +1 frame/lvl|Swift Nock: nocking costs -8%/lvl|Elven Cloak: after a True Shot take -6%/lvl damage for 2s|
|2 (14/15/16)|Steady Hand: True Shots +5%/lvl damage per streak step|Fletcher: a True loose of a nocked arrow refunds 10%/lvl of its cost|Point Blank: True Shots knock foes within 48px back 24px and stun 0.5s +0.1s/lvl|
|3 (20/21/23)|Unerring (max 1): at streak 5 every True Shot also fires a free Piercing Shot line at half power|Twin Nock (max 1): a True loose of a nocked arrow fires it twice (second at half)|Unshaken (max 1): hits never break your draw (no flinch while holding B)|

### Tests (when built)
Glint appears at 30 frames, True only inside the window, streak colors, reset on a normal shot. Each nocked arrow
fires on the loose, half on normal, full on True. Swap refunds. No bow: "Needs a bow". Co-op: each player's own glint.

### To RE / risks (Ghidra is down, confirm before building)
- Does the Overdraw shot fire on B release or on its own? The design needs "release fires". If it fires on its own,
  the glint moves to that moment and the window becomes "keep holding until the glint".
- Whether stock arrow hits reach resolveRangedHit with the player as attacker (needed for the +25% True bonus).
- Unshaken: where the draw gets cancelled by a hit (flinch path).
