# Gauntlet prototype

Open The Gauntlet from the main menu, choose a hero, and begin a run.
The briefing shows the selected map and enemy family before entering.
After rewards and any queued level upgrades, another briefing previews the next room.

## Inside a room

Combat rooms have two or three waves depending on their door type.
After a short arrival pause the first wave warns, then its foes arrive just off screen and come for the hero.
Entry positions are randomly chosen from twelve points sampled across each map's connected floor routes.
Candidates must fit the hero and a wider clearance box; the native mission entry is the fallback.
The entry is selected during player creation, before native camera setup and terrain rendering.
A breather follows each wave before the next one.
The compass points at the nearest living foe during a wave and holds its bearing between waves.
Foes appear on a ring around the hero, on floor they can walk straight to the hero from.
The living-enemy cap remains five to eight as the run progresses.

After the final wave, the game leaves a short settling period for the last kill and lingering effects.
If another living foe appears during that period, combat resumes.
The room-clear message then appears before healing and the reward screen.

Gauntlet ignores Max Level, extra skill/attribute point, and Max Kills cheats during a run.
Their saved settings remain available in campaign play. Combat cheats retain their selected settings.

## Doors

After the rewards, a door screen offers three doors, drawn with the stock bonus-map icon art and its cursor frame.
Each card shows its kind, reward, and modifier.
Battle cards show "+R" for their additional boon reroll, and the door details and briefing spell out the reward.
Skirmishes give a boon after two waves; Battles add a reroll for completing the extra wave.
Text preserves neighboring pixels when compact labels or paragraph lines share screen tiles.
Door artwork keeps its native 32 by 64 size, with rewards on a separate row below it.
Modifiers also appear in the selected door description, which is bounded to its panel.
The selected door's details name its map, foes, and room rules.
The upper-right room counter shows the room being chosen, including noncombat rooms.

| Door | Room | Reward |
| --- | --- | --- |
| Battle | Three waves of one foe family on a map. | A boon and one boon reroll. |
| Elite | Champions lead two of the three waves, one tier tougher and one foe larger. Available from room 3. | A rare or better boon and a herb. |
| Rest | No fight. Offered when the room number is a multiple of 4. | Three fresh herbs, capped at ten. Heroes without Herbal Healing receive one boon reroll instead. |
| Treasure | No fight. Available from room 3 as a random special door. | A rare or better boon. |
| Skirmish | Two waves with shuffled encounter roles. | A boon. |
| Onslaught | Three ambush waves with enemies arriving from all sides. | A rare or better boon. |
| Trial | Two champion waves, two enemy tiers tougher (capped at the highest tier). Available from room 6. No room-clear healing, including Mending. | An epic or better boon. |
| Shrine | No fight. A random special door. | Random armor, cloak, gloves, shoes, or accessory, scaled to run depth. Full inventory converts the reward to one boon reroll. |

Door positions are shuffled on each offer.
One Battle door is always available, alongside a Skirmish, Elite, or late-run Trial and a special room.
Every fourth room offers Rest.
Between those milestones, the special slot randomly offers Treasure, Shrine, or Onslaught, with Onslaught twice as likely as either other choice.
At 40% HP or less, there is also a one-in-three chance of an extra Rest offer.
Treasure and Shrine do not immediately repeat after taking the same special door.
The foe family is chosen per door and is independent of the map, so the same map can host different foes.
Men of the East appear from room 4.
Rests and treasures count as rooms.

## Gear shop

Press Select on the door screen to visit the shop, then B to return to the same doors and selection.
Shopping does not consume a room or reroll doors.
Four offers are generated once per room: a hero-compatible weapon, two random armor pieces, and an accessory.
Stock refreshes after each completed room, including noncombat rooms.
Purchased offers remain sold until the next refresh, and reopening the shop never rerolls stock.

Use Left/Right or L/R inside the shop list to choose the Buy or Sell tab.
Select also toggles the tabs.
Item rows show native backpack artwork, equipment type, minimum level, price, and sold or unaffordable status.
The selected row and active tab share a warm highlight, and the wallet uses a gem badge.
Use Up/Down to choose an offer or backpack slot, and A to inspect it beside the currently equipped item.
The inspection shows price, minimum level, and both items' stats.
On inspection screens, Up/Down browses other items without returning to the list, skipping empty backpack slots and sold shop offers.
Right or R advances long stat descriptions; Left or L returns to their beginning.
The inspection shows both items' artwork and previews the gem balance after the transaction.
A on the inspection screen buys the item into the backpack; B returns without buying.
A full backpack or insufficient gems rejects the purchase without charging or consuming stock.
Equipment is not automatically swapped when buying.

Base items are filtered by run depth, class permissions, and current equip level.
Affixes that make an item unusable are discarded.
Buy prices start at 12 gems, plus 2 per room, 6 per base tier, 3 per prefix tier, and 4 per suffix tier.
The Sell tab covers all eight backpack slots, four at a time; equipped slots are protected.
Inspecting an item shows its payout, and a second A confirms the sale; B cancels.
Sale values are 8 gems plus 5 per base tier, 2 per prefix tier, and 3 per suffix tier.
Arrow stacks sell for 2 gems plus one per five arrows.
Sale values do not increase with room depth and remain below the buy price of the same ordinary equipment.
A sale clears only the confirmed item, removes carried-passive bonuses when applicable, and rejects a full gem pouch without removing anything.
Smeagol cannot equip native gear, so his shop explains the restriction instead of offering unusable items.

## Room modifiers

From room 3, combat doors other than Trials can also carry one modifier.
The card names it before entry, and the briefing describes its effect and reward.
Modifiers apply only to that room.

| Modifier | Change | Reward |
| --- | --- | --- |
| Armored | All spawned enemies gain 25% HP, after any champion bonus. | Rare or better boon. |
| Rush | Breaks between waves drop from 90 to 30 ticks. Arrival and warning pauses stay the same. | Rare or better boon. |
| Supplies | Normal combat and healing. | Two extra herbs on clear, or 25% maximum mana for heroes without Herbal Healing. Pouch and mana caps apply. |

Supplies preserves mana already above its normal maximum.
Trials do not have room modifiers.
Twenty-room mixed-door regressions passed for all ten heroes, including Trial entries and all three modifiers.
Separate debugger checks confirmed the Armored HP increase, Supplies mana recovery, and Trial healing restriction.
The living-enemy cap is unchanged for all these variants.

## Herbs

Heroes that have Herbal Healing (every hero except the Ringwraith and Saruman) start the run with it and three fresh herbs.
Each rank after 1 adds healing equal to 10% of maximum HP per herb, on top of the herb's normal healing: +10% at rank 2, +20% at rank 3, +30% at rank 4, and +40% at rank 5.
A successful use still consumes one fresh or dried herb, costs no Spirit, and cannot heal above maximum HP.
Campaign upgrades unlock at hero levels 5, 10, 15, and 20.
The pouch carries over between rooms and holds ten.
New runs clear both native and expanded skill ranks before granting starters and hub bonuses. Frodo also retains his native starting Ring ability.
Starting skill controls are initialized for both cast cycling and quick select. The initial selection is a learned offensive skill, and starting quick slots never point to unavailable skills.
Rest doors add three fresh herbs; elite rooms add one.

## Encounter roles and rewards

Battle rooms shuffle Patrol, Ambush, and Elite Guard across three waves.
Skirmishes use two of those shuffled roles.
Elite rooms use Elite Guard, Ambush, Elite Guard; Onslaughts use three Ambush waves.
Patrols have three to five defenders with mixed archetypes where the enemy pool permits.
Ambushes give a longer warning, then release four to twelve defenders from offscreen floor in quick succession, subject to the living-enemy cap.
Elite Guards have one defender drawn from the strongest available tier, up to one tier above the room's normal limit, with 50% extra HP, plus two to five support enemies.
Wave announcements identify the encounter role.

Reward details show the effect's numbers and current ownership or stat/skill rank before and after choosing it.
Boons are unique (one copy maximum), rather than stacking duplicates.
Rare and epic door rewards prefer their advertised rarity.
If no eligible unowned boon remains at that rarity, the offer lowers its rarity floor until a boon is available.
The reward screen labels this with "Rare pool empty" or "Epic pool empty".
When every eligible boon is owned, the run proceeds without another boon.
Long descriptions stay inside the detail panel. Right advances to more text and Left returns to the beginning.

## Boons

The pool contains 94 unique boons: the adapted 80-idea catalog plus fourteen additional boons.
Eight duos require both parent boons, and ten hero-specific offers require the matching hero.
See [the boon catalog](gauntlet-boons.md) for exact effects and prerequisites.
One third of reward rolls try to include an eligible boon that complements an owned boon, without changing the rarity floor, prerequisites, uniqueness, or one-legendary limit.
The reward screen labels those choices as build synergies.
Press L on the door screen to see both evolving boons' progress and upgraded effects, then B or L to return to the same selected door.

Status and cast workspaces are allocated only when an owned boon needs them and freed before room transitions.
Eight enemies can carry run statuses, four secondary or delayed targets can wait at once, and ghost summons share a cap of four.
These limits keep encounters bounded on the GBA. Optional effects refuse new work when their pool is full.

## Map coverage

Twenty-eight maps are in the rotation, with twelve entry candidates per map sampled from connected floor.
Door pictures are shared room-kind or enemy-theme artwork, not individual map previews.
Normal selection avoids the current map and duplicate combat maps within one door set, but does not keep a run-wide visited-map history.
Each entry is checked against the current hero and a 49 by 38 pixel clearance box, with nearby floor adjustments up to 16 pixels.
Coordinates are stored in [gauntlet_routes.c](../hacks/patches/gauntlet/gauntlet_routes.c).

| First available room | Maps |
| --- | --- |
| 1 | Isengard, Flooded Field, Fangorn, Moria, Treasure Hoard, Balin's Tomb, Ice Caves, Gorgoroth, Gorgoroth Hills, Ithilien, Cirith Ungol, Ungol Stairs, Fangorn Copse, Entwash |
| 2 | Dimholt Glen, Uruk Hatchery, Orthanc Base, Edoras Fields |
| 3 | Mount Doom Foot, Mount Doom Slopes, Dark Paths |
| 4 | Anduin, Anduin Docks, Dunharrow, Corsair Deck, Corsair Hold, North Fields, Haradrim Camp |

Helm's Deep Valley and Morgul Vale remain outside the rotation because their reachable entrance areas did not provide
enough open floor with story scripts suppressed. Their route entries are empty, and both random selection and the map probe skip them.
Story scripts and campaign warps remain suppressed during a run.
Animated-water backdrops are initialized separately using the native water effect.
Fangorn, Entwash, Anduin, and Corsair maps need this visual setup even when campaign scripts are disabled.
The water fix passed 27 room clears with Aragorn, Ringwraith, and Saruman across all seven animated-water maps and two other maps, including water-to-dry transitions.
This prototype uses existing terrain with its own encounters, rather than restoring each map's campaign objectives.
An empty first route excludes a map from both normal rotation and the probe.

## Regression runs

```sh
uv run python hacks/tools/gauntlet_run.py --emulator C:/path/to/mGBA.exe   --heroes 2 --rooms 30 --probe --jobs 1 --output build/gauntlet-runs/maps
uv run python hacks/tools/gauntlet_run.py --emulator C:/path/to/mGBA.exe   --heroes 0 1 2 3 4 5 6 7 8 9 --rooms 12 --jobs 2 --output build/gauntlet-runs/heroes
```

The automated player clears waves through the damage path and exercises the door screen (it walks the cursor across
all three doors), rewards, briefings, results and the hub.
The probe takes the first door of each set so it visits every map in order.
Each combat entry also logs its room type, map, selected candidate, coordinates, and terrain clearance.
A `GAUNTLET_VARIANT` line records its modifier and minimum boon rarity.
A `GAUNTLET_BACKDROP` line records the mission, native backdrop effect, BG1 control, and animation frame at room clear.
The runner rejects blocked player entries alongside enemy placement failures and emulator faults.
The earlier 30-map pool passed combat and placement probes, but those checks did not establish visual correctness.
City Streets and Minas Tirith (Gates) are excluded after visual captures reproduced corrupt terrain.
These exclusions avoid broken rooms; their native terrain initialization is not repaired.
Seven additional heroes passed mixed-door runs covering Battle, Elite, Skirmish, and Onslaught.
Shrine equipment uses the native base-item, prefix, suffix, and inventory routines.
These runs do not replace manual checks of enemy pursuit, encounter balance or the feel of a room.

The native-font regression runs with `uv run python hacks/tools/test_gauntlet_text.py` and is included in `test_ability_support.py`.
It reproduces clipped neighboring glyphs, checks pixel preservation, and checks a crowded door layout with the original ROM font against the text tile budget.

The boon host regression runs with `uv run python hacks/tools/test_gauntlet_boons.py` and is also part of `test_ability_support.py`.
Use `--boons` with `gauntlet_run.py` to grant all boons and exercise starting-skill casts in stress runs.
This deliberately bypasses normal hero and duo offer restrictions. Normal ROM builds keep that test mode disabled.

Aragorn retains accumulated Flame charge through stat rebuilds for the same living hero, including level-ups.
