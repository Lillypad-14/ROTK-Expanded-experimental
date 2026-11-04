# How the stock heroes' swings are built (reference for new attack animations)

Measured with `hacks/tools/vanilla_study.py` (run it for the numbers, `--sheets` for pictures).
Studied 2026-11-02 after the Witch-king's swings, built from his boss sprite, kept looking janky.

## What the stock swings do

- **Parts, not one sprite.** Every stock hero draws a pose as 8 parts (body, arms, weapon, ...)
  animated together (bank `SpriteAnimation *[8 parts][7 poses]`, `HeroMeleeAnimationBanks`).
  The weapon is its own sprite, so it can sweep smoothly while the body leans.
- **A new drawing every tick.** Swings have no held frames at all (Aragorn swing 1: 12 ticks, 12
  drawings). Holds appear only in recoveries and in long finishers (Aragorn's: 27 drawings, 9 held).
- **Small, steady body motion.** The body moves 1 to 2 px per tick on average, at most 3 to 8
  (Smeagol, who hops). Never a 10+ px jump.
- **Whole-body acting.** Aragorn swing 1: rear back over 2 to 3 ticks with the sword raised behind,
  arc forward over 3 ticks with a deep forward lean, follow through low for 4 ticks. The cape follows.
- **The weapon passes through ~10 evenly spaced positions** per swing, and the fastest move is the
  strike.
- **Swings chain by pose.** Swing 1 ends low in front exactly where swing 2 (a rising cut) begins.
- **No smears, trails or flashes.** The smoothness is all in the drawings.
- **Small sprites.** Heights 35 (Frodo, Sam) to 65-80 px (Aragorn, Gandalf's finisher). A boss
  sprite (the Witch-king, ~80 px) needs proportionally small moves per tick to read as smooth.
- **Hits come from code, not from frames.** Stock swing frames carry no hitbox. Enemy-bodied heroes
  need an explicit hit frame (`"hit": true` or a box).

| hero | swing 1 | recovery 1 | swing 2 | recovery 2 | finisher (ticks; screen frames = x2) |
|---|---|---|---|---|---|
| Frodo / Sam | 8 | 5 | 11 | 6 | 13 |
| Aragorn | 12 | 8 (4 drawings) | 10 | 8 (4) | 36 (27) |
| Gandalf | 15 | 5 | 9 | 6 | 44 (33) |
| Eowyn | 11 | 12 (6) | 9 | 10 (5) | 24 (18) |
| Gimli | 12 | 8 | 9 | 7 | 16 |
| Smeagol | 11 | 10 | 11 | 10 | 13 |

## Why the boss-sprite swings could not get there

The Witch-king's Attack clip is 17 drawings of one enemy attack, made as a single sprite. A three-swing
combo from it needs holds (stutter: the body stops, then jumps) and out-of-order frames (pops), and
the sword cannot move without the body. Smears hid some of it but added pop-in. Every facing is
its own drawing, so fixes for one facing do not carry to the others.

## What new attack animations need

1. A separate weapon (or arm and weapon) that moves every tick along an even arc, over a body that
   leans back, lunges and settles in small steps.
2. No holds inside a swing. Hitstop is at most 1 to 2 ticks on the impact.
3. Each swing ending in the pose the next one starts from.
4. All five facings designed, the down (front, facing 4) and up (back, facing 0) ones included.
5. Effects optional and small. Stock heroes have none.
