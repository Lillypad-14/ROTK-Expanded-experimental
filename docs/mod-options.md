# Mod options

Open **Options > Mod Options** from the title screen. Press L/R to switch pages, Up/Down to pick a row and
Left/Right to change a value. Help text for the highlighted row appears at the bottom of the screen. Settings are
saved in the global mod options area and apply to every save slot.

In co-op, both players play with the host's (player 1's) Loot QoL, Debug cheats, Twists and Experimental settings, so
both consoles simulate the same game. The other player's save keeps its own settings. Mod options can't be changed
during a co-op session: their values show greyed out, with a note in place of the help text.

## Pages

| Page | What it holds |
| --- | --- |
| Loot QoL | Loot filter and auto-collect gems ([upstream guide](romhack-user.md#loot-quality-of-life)). |
| Unlocks | Bonus maps, Sam, Smeagol and every hero path in Chapter Select. |
| Debug | Max level, infinite skill and attribute points, god mode, infinite Spirit, one-hit kills, max kills. |
| Twists: Combat | Damage modifiers. See below. |
| Twists: Vigor | Healing and resource modifiers. See below. |
| Twists: Fate | XP and random events. See below. |
| Performance | Off-by-default performance experiments. See [performance](performance.md#experimental-audio). |
| Performance 2 | Effect Cap. See [performance](performance.md#effect-cap). |
| Combat Lab | Combat display and test encounters. See [test arena](test-arena.md). |
| Ability Bench | Cast any skill at any level. See [Ability Bench](ability-bench.md). |
| Delivery Test | Fire projectile and effect deliveries. See [skill deliveries](skill-deliveries.md). |
| Effect Preview | Show a skill effect on the player. See [skill effects](skill-effects.md). |

The Debug, Combat Lab, Ability Bench, Delivery Test and Effect Preview pages are testing tools.

## Twists

Twists are playthrough modifiers. They stack with each other and apply in single player and co-op.

### Combat

| Option | Values | Effect |
| --- | --- | --- |
| Damage Dealt | 50% to 300% | Scales all damage you deal. |
| Damage Taken | 25% to 300% | Scales enemy attack damage. Extra damage never kills on its own. |
| Life Drain | Off, 5%, 10%, 20%, 35% | Heals a share of the damage you deal on each hit. |
| Lucky Strikes | Off, 5%, 10%, 20%, 33% | Chance for a hit to deal double damage. |
| Berserker | Off, On | Up to +100% damage as your HP falls. |
| Momentum | Off, On | +5% damage per kill, up to 10 kills. Lost when you are hit or after 5 seconds without a kill. |

### Vigor

| Option | Values | Effect |
| --- | --- | --- |
| HP Regen | Off, 1%/s, 2%/s, 4%/s | Regain a share of max HP every second. |
| Spirit Regen | Off, 2%/s, 4%/s, 8%/s | Regain a share of max Spirit every second. |
| Kill: Heal | Off, 3%, 6%, 12%, 25% | Each kill restores a share of max HP. |
| Kill: Spirit | Off, 5%, 10%, 20%, 40% | Each kill restores a share of max Spirit. |
| Party Share | Off, On | Kill rewards also go to your co-op partner. |
| Wound Lock | Off, On | Taking a hit pauses HP regen for 5 seconds. |

### Fate

| Option | Values | Effect |
| --- | --- | --- |
| XP Gain | 50% to 500% | Scales all XP from kills and rewards. |
| Omens | Off, On | Each level rolls a random omen, announced when the level starts. |
| Ring's Whisper | Off, On | Every 10 kills: 10 seconds of +50% damage dealt and +50% damage taken. |
| Fellowship | Off, On | Co-op only. Near your partner: +15% damage and -15% damage taken. Far apart: -15% damage. |

Omens:

| Omen | Effect for that level |
| --- | --- |
| Blood Moon | +50% damage dealt and taken. |
| Fortune | Lucky Strikes at least 25%. |
| Vampire Night | +15% Life Drain. |
| Bounty | +100% XP. |
| Hearth | +2%/s HP regen. |
| Iron Skin | -35% damage taken. |
| The Hunt | Momentum on. |

## Adding a page

Pages are registered in `ModOptionsPages` in
[`mod_options.c`](../hacks/patches/mod_options/mod_options.c). The entry and page structs are documented in
[`mod_options.h`](../hacks/patches/mod_options/mod_options.h). The menu reuses the stock options screen, so text has
fixed space:

- **Title:** about 14 characters, like "Effect Preview". Longer titles run into the "Page x/y" counter.
- **Rows:** at most 8. The label and value of one row share 25 glyph tiles, so keep labels short (like
  "Infinite Skill Pts") and values to a word or two.
- **Help text:** two lines of about 26 characters, split with `\n`. It is drawn on rows 6 and 7, so a page with help
  text should have at most 6 entries.
- **Saved values:** a page's settings live in the global mod options area. Add new fields at the end and keep existing
  fields in place, so older saves still load.

Check every new page in mGBA. Text that does not fit shows up as cut-off words or stray glyph fragments on other rows.

## Hero kits

A character of one of the 8 stock heroes (Aragorn, Legolas, Gimli, Gandalf, Frodo, Sam, Smeagol,
Eowyn) plays its Expanded kit or its vanilla one, chosen with L when it is created: on the new game
hero select (top right: "L: Expanded kit" / "L: Vanilla kit") and on the save menu's hero select
(bonus maps, co-op), as the slot's third line. A vanilla character plays its stock skill rows and
passives only: no new rows or page 2, none of its hero's Expanded mechanics (Gandalf's forms,
Legolas's quiver, Gimli's Tally, Frodo's Burden and Snare, Aragorn's Flame of the West and Elite
passives, Sam's plants, Smeagol's tricks, Eowyn's Against the Dark) and its stock casts unwrapped.
The kit belongs to the save slot: bit 0 of its skill record's `reservedHeader[0]`, so existing
saves stay Expanded. Saruman and the Ringwraith exist only in Expanded and have no choice. In co-op
each player's own character keeps its kit; the page 2 record carries it to the partner's console.
