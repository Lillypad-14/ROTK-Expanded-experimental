# Heroes

What each hero plays like in ROTK Expanded: the idea, the unique mechanic, and the skills on each page.
Numbers here are the design values. The source of truth for every skill's text, cost and level is
[`skill_expansion_content.c`](../hacks/patches/skill_expansion/skill_expansion_content.c), and the per-hero logic
lives in the kit files linked under each hero.

## How the expanded skill tree works

- Every expanded hero keeps the stock page 1 and gains a **page 2**: up to six more active skills and nine more passives.
  Press **L / R** on the skill tree to flip pages.
- Each page is laid out as **three columns, one per build**, color-coded on the tree.
- Page 2 skills unlock between hero levels 10 and 25.
- From displayed level 21 on, each level grants **two skill points** instead of one, so late-game heroes can fill
  both pages.
- Page 2 actives go in the same quick-select slots as page 1 actives.
- Most heroes have a **HUD bar** between the HP and Spirit bars that shows their mechanic.

Status key: **tested** means played in an emulator with that ROM build. **Untested** means it builds and passes the
host checks but has not had a manual gameplay pass.
All ten heroes completed the automated ability and Gauntlet checks in the [2026-10-25 audit](bug-audit.md); those checks do not certify every move, visual effect, or balance choice.

## At a glance

| Hero | Mechanic | One-line idea | Status |
| --- | --- | --- | --- |
| [Aragorn](#aragorn) | Flame of Anduril | Build an orange Flame bar through hits, stuns and marks, then spend it on the King's abilities | tested |
| [Frodo](#frodo) | Burden | The Ring's corruption is a visible meter: power that rises in combat and costs you if it peaks | tested |
| [Sam](#sam) | Samwise's Garden | Plant taters and brambles that grow, ripen and wilt. Ripe taters pelt foes, harvests restore Spirit | tested |
| [Smeagol](#smeagol) | Gollum's Grip | Grip builds toward 100. Then Gollum takes over for two seconds at double damage | tested |
| [Gimli](#gimli) | The Tally | Page 2 is bought with kills. Every 10 kills charges a free, stronger cast | tested |
| [Gandalf](#gandalf) | The Grey and the White | Hold R to swap form. Grey saves Spirit, White burns it for power | automated emulator checks; manual pass pending |
| [Legolas](#legolas) | The Glint | Release Overdraw on the glint for a True Shot. Chain them into a streak | automated emulator checks; manual pass pending |
| [Ringwraith](#ringwraith) | Dread | New hero. Terrify and stun foes, then raise the slain as thralls | mostly tested |
| [Saruman](#saruman) | Empowered spells | Cast three spells, then empower the next damaging spell | host and automated emulator checks; manual pass pending |
| [Eowyn](#eowyn) | Against the Dark | Odds rise with danger. Rohirrim can build Gallop and spend it on a strike | experimental; manual pass pending |
| [Witch-king](#witch-king) | The Black Captain's dread | PC only. Iron Crown turns blows back on the striker, Lord of Terror makes foes cower every second, No Living Man cheats death. | headless checks; manual pass pending |
| [Orc Captain](#orc-captain) | The lash | PC only. Whip lines, a boot that sends foes flying, taunts, a war-banner haste and a cleave that cuts everyone around him. | headless checks; manual pass pending |
| [Uruk-hai](#uruk-hai) | Bred in the Pits | PC only. Rises from the mud when slain. Whelps, devouring the wounded, a mud hide and the White Hand's lifesteal. | headless checks; manual pass pending |
| [Guard of Gondor](#guard-of-gondor) | The city's defence | PC only. Calls guards to his side, a shield wall, the Horn of Gondor, a Swan Knight charge and the Eagles. | headless checks; manual pass pending |
| [Denethor](#denethor) | Fire and despair | PC only. Pays health for spell power, sets foes burning, the White Tower turns arrows, madness trades defence for damage. | headless checks; manual pass pending |
| [Mouth of Sauron](#mouth-of-sauron) | Treachery | PC only. Turns a foe against its friends, curses foes to take more damage, rides them down; the Eye calls fire on six. | headless checks; manual pass pending |
| [Army of the Dead](#army-of-the-dead) | Terror and the grave | PC only. Foes flee in terror, he passes through them as mist, grave fog and soul harvest; the grey host sweeps the field. | headless checks; manual pass pending |
| [Warg](#warg) | Frenzy | PC only. Every bite stacks Frenzy (a HUD meter). Leaps, howls that weaken, bleeding bites and a throat-bite finisher. | headless checks; manual pass pending |
| [Spider](#spider) | Web traps | PC only. Webs that hold foes, stacking venom that paralyzes, reels foes in, cocoons them; Shelob's Lair rings her in webs. | headless checks; manual pass pending |
| [Corsair](#corsair) | Plunder | PC only. Kills drop loot. A grappling hook, cutlass flurry, blinding smoke powder, powder kegs and a broadside of cannon. | headless checks; manual pass pending |
| [Uruk Crossbowman](#uruk-crossbowman) | The marksman | PC only. His basic attack shoots bolts. Piercing bolts, volleys, a pavise to stand behind, fire that spreads, steady aim, a rain of bolts. | headless checks; manual pass pending |

---

## Eowyn

**Against the Dark:** her page-two Odds meter reacts to nearby threats, larger foes, Nazgul and missing health. It moves through Steady, Grim, Desperate and Hopeless tiers. Rohirrim can learn Gallop, building steps by running and spending them on a melee hit.

The six active skills are Death-Seeker's Strike, Come and Face Me!, Lance of the Mark, Stand Over the Fallen, Hope Rekindled and I Am No Man!. They add an HP-cost strike, pull/stun, Gallop lunge, defensive ally support, Odds-based healing and a high-danger ultimate. Nine new passive IDs support the kit.

Definitions and level requirements are in [skill_expansion_content.c](../hacks/patches/skill_expansion/skill_expansion_content.c), with behavior in [skill_expansion_eowyn.c](../hacks/patches/skill_expansion/skill_expansion_eowyn.c). The kit has its own icons. It builds and passes content checks, but complete manual combat, balance and co-op testing remain pending. Earlier automated audits predate this kit.

## Aragorn

**Idea:** the Ranger who becomes King. Page 2 is about earning momentum in a fight and cashing it in.

**Mechanic: the Flame bar.** An orange HUD meter, Anduril's Flame, fills from melee and projectile hits, stuns,
ambush strikes, ghost hits and hits on marked foes. It powers his page 2 skills and passives.
While the bar is full, Anduril glows with fire and every melee blow adds 4 fire damage. Flame of the West gives the blade a
sun glow of its own.

**Builds (page 2):** Ranger | Dead | King.

| Actives | |
| --- | --- |
| Flame of the West | Spends the Flame bar. Anduril blazes and every hit scorches the foe it lands on and the foes around it (3 + 3 per level, doubled from a full bar), for longer from a full bar. |
| Ranger's Ambush | Go unseen. The next blow crits harder, stuns and stokes Flame. |
| Tracker's Mark | Marks the nearest foe for bonus damage and double Flame. Its death stokes more. |
| Oath of the Dead | Your ghosts burst, stunning foes and stoking Flame. With no ghosts it raises one. |
| Return of the King | Anduril is Reforged. All skills gain a level and cost no Spirit for a while. |
| Paths of the Dead | The Army of the Dead charges ahead, hitting and stunning every foe in its path. Spends the Flame bar (+50% from a full bar) and grows 20% per ghost of his, up to three. |

Passives: Strider's Riposte (counter when an attack fails to land), Restless Dead, King's Presence (longer stuns),
Hunter's Patience (a waited-for strike builds Flame), The Dead Ride, The Blade Reforged (doubles flame bursts),
Track the Quarry, Lord of the Dead, Hands of the King.

Page 1 upgrades from page 2 passives include +8 to Sweep and +10 to King's Command.

Code: [`skill_expansion_elite.c`](../hacks/patches/skill_expansion/skill_expansion_elite.c).

## Frodo

**Idea:** the Ring is both your strongest weapon and your biggest danger.

**Mechanic: Burden.** The stock game already tracks Frodo's corruption. Expanded makes it a HUD bar and builds a
kit around it:

- Burden rises in combat and falls outside it. Hits from Nazgul add Burden.
- Burden now carries over between zones instead of resetting.
- The bar is gold below 51, red from 51 to 90, and pulses from 91 up.

**Builds (page 2):** Good | Evil | Tricks.

| Actives |
| --- |
| Phial of Galadriel, Elbereth!, Shadow of the Ring, Sow Discord, Smoke and Mirrors, Put On the Ring |

Passives: Strength of Will, Weight of the Ring, Riddles in the Dark, Sting Glows Blue, The Ring's Hunger,
Tricksy Hobbit, Unwavering, Claim It, Precious Lies.

Code: [`skill_expansion_frodo.c`](../hacks/patches/skill_expansion/skill_expansion_frodo.c).

## Sam

**Idea:** a gardener and cook who wins fights by preparing the ground.

**Mechanic: Samwise's Garden.** Sam plants taters and brambles that grow, ripen and wilt over time.

- Ripe taters pelt nearby foes.
- Harvesting a ripe tater restores Spirit.
- Bramble hedges snag nearby foes every second and spread wider as they grow.

**Builds (page 2):** Gardener | Stout | Cook.

| Actives |
| --- |
| Plant Taters, Elven Rope, Second Breakfast, Bramble Hedge, Frying Pan Parry, Po-tay-toes! |

Passives: Green Thumb, Stubborn, Lembas Crumbs, Gaffer's Lore, Shire Grit, Fine Seasoning, Master Gardener,
I Can Carry You, Taters for All.

Code: [`skill_expansion_sam.c`](../hacks/patches/skill_expansion/skill_expansion_sam.c).

## Smeagol

**Idea:** two personalities in one body. Smeagol plays sneaky and nice, and Gollum is always waiting.

**Mechanic: Gollum's Grip.** A meter from 0 to 100 that builds as you fight.

- At 100, Gollum takes over for 120 frames (2 seconds). He steers himself toward foes and attacks at double damage.
- Mashing A cuts the takeover short.

**Builds (page 2):** Smeagol | Gollum | Precious.

| Actives |
| --- |
| Play Dead, Throttle, Drop the Precious, Catch a Fish, Slink, Argue With Yourself |

Passives: Poor Smeagol, Hungry, Sniff It Out, Nice Master, Rat Catcher, Finders Keepers, Smeagol Is Free,
We Wants It, My Precious!.

Code: [`skill_expansion_smeagol.c`](../hacks/patches/skill_expansion/skill_expansion_smeagol.c).

## Gimli

**Idea:** the dwarf who keeps count. His second page is earned by fighting, not by levelling.

**Mechanic: The Tally.**

- Page 2 ranks are bought with **kills** as well as points. Every 10 kills is a mark, and each rank needs another
  step of 20 kills.
- A charge bar fills from kills. At 10 kills, his next cast is **free and deals +50% damage**.

**Builds (page 2):** Stone | Fury | Tally.

| Actives |
| --- |
| Baruk Khazad!, Cleave the Weak, Nobody Tosses a Dwarf, Hold the Deep, Book of Grudges, Still Counting |

Passives: Dwarven Mail, Grudgebearer, Friendly Wager, Not Without Me, Blood on the Axe, Orc-Count,
Stubborn as Stone, Durin's Folk Don't Fall, Forty-Two!.

Code: [`skill_expansion_gimli.c`](../hacks/patches/skill_expansion/skill_expansion_gimli.c).

## Gandalf

**Idea:** the Grey Pilgrim and the White Rider are two different wizards. Pick the right one for the moment.

**Mechanic: The Grey and the White.** Hold **R for half a second** to swap form.

- **Grey** regenerates and saves Spirit.
- **White** has no Spirit regen and drains 3 Spirit per second, but page 1 skills deal +30%.
- Swapping is itself a move: returning to a form stuns nearby foes, and the smoke of a swap makes enemies lose track
  of you.
- Each page 2 row has a **Grey half and a White half**. The form you are in decides which one you cast.

**Builds (page 2):** Grey Pilgrim | White Rider | Narya.

| Row | Grey | White |
| --- | --- | --- |
| 7 | Fireworks | Flame of Anor |
| 8 | A Wizard Is Never Late | Shadowfax |
| 9 | Smoke Rings | Breaking of the Staff |
| 10 | Pipe-weed Pause | Healing of Theoden |
| 11 | Fly, You Fools! | Look to My Coming |
| 12 | You Shall Not Pass! | Light of Valinor |

Passives: Wandering Wizard, Robes of White, Kindler of Hearts, Old Tricks, Unbending, Quickened, The Fall,
The White Rider, Servant of the Secret Fire.

Code: [`skill_expansion_gandalf.c`](../hacks/patches/skill_expansion/skill_expansion_gandalf.c).

## Legolas

**Idea:** a timing game. Anyone can shoot. An elf hits the exact moment.

**Mechanic: The Glint.** Hold **B** to Overdraw.

- About 12 frames after full draw, a glint flashes on Legolas. Release inside that window for a **True Shot**,
  which deals +25% damage.
- True Shots chain into a streak from 1 to 5. The glint changes color as the streak climbs, and a 5-streak calls
  out "Unerring!".
- Page 2 actives are **nocked arrows**. Loosed as a True Shot they do their full effect, and loosed normally they
  do half.

**Builds (page 2):** Eye | Quiver | Grace.

| Actives |
| --- |
| Piercing Shot, Shield-Surf, Pinning Shot, Gift of the Galadhrim, Volley of Helm's Deep, Still Only Counts as One! |

Passives: Elf-Sight, Swift Nock, Elven Cloak, Steady Hand, Fletcher, Point Blank, Unerring, Twin Nock, Unshaken.

Code: [`skill_expansion_legolas.c`](../hacks/patches/skill_expansion/skill_expansion_legolas.c).

## Ringwraith

**Idea:** play as one of the Nine. A ninth playable hero (hero id 8), built from the Nazgul enemy model, with its
own skills, stats, palette and gear.

**Mechanic: Dread.** The Ringwraith fights through fear.

- Its skills terrify and stun foes.
- Slain foes can rise as **thralls** that fight for it.
- It gains Spirit only from its kills, not from regeneration or pickups.
- Spirit starts each mission at a quarter (full with Ringbound), and the meter shows it from the first frame
  after a respawn.
- Loading a save restores full health and clears Burden, the stock corruption meter.

**Three builds:** Thralls (raise and command the dead), Assassin (shadowstep and knife work), Fear (terror and
control).

| Page 1 actives | Page 2 actives |
| --- | --- |
| Morgul Blade, Cry of the Nazgul, The Unseen World, Fell Beast, No Living Man, Black Breath | Command of the Nine, Shadowstep, Shadow of Mordor, Black Sacrifice, Will of Sauron |

**Fell Beast** is a visible dive. A dark steed made from the stock flying Nazgul crosses the screen in front of the
wraith (left or right, by facing) and strikes each foe in its band once as it passes, with the damage, terror and
stun of the old instant sweep. The Fell Rider's Spurs double the band and terrify. The beast is a four-piece
144x128 sprite, see [skill effects](skill-effects.md#large-effects).

**Thralls** drop a swing the moment their target dies or leaves their reach, then turn on the nearest living foe.
The stock ghost AI finishes its whole swing first, which left thralls standing still hitting nothing.

**Icons:** all 29 skills use full-color custom art (see the icon recipe in
[`howto-heroes-skills.md`](wip-docs/howto-heroes-skills.md)).

Page 1 passives: Servant of the Ring, Shadow Hunger, Wraith Form, Fear's Reach, Lord of Terror, Witch-king's Mace,
Morgul Sorcery, Undying Malice, Ringbound.

Page 2 passives: Fading, Host of Angmar, Dread Lord, Feast of Fear, Grave Pact, Knife in the Dark,
Master of the Dead, Death from Shadow, Endless Night.

**Wraith gear:** 8 base item families and 11 uniques, plus wraith prefixes and suffixes. The Ringwraith's drops
roll from wraith gear, and other heroes never roll it.

Code: [`hacks/patches/ringwraith/`](../hacks/patches/ringwraith/) (kit in `ringwraith_kit.c`, thralls in
`ringwraith_thralls.c`, gear in `wraith_gear.c`).

## Saruman

**Idea:** the White Hand, before and after the fall. A tenth playable hero (hero id 9) using Saruman's boss graphics and Gandalf's equipment mapping, who
replaces both skill pages.

**Mechanic: Empowered spells.** Three successful casts fill one gold meter. The next damaging spell deals +50% damage and empties it.

- Repeating a spell counts. The third cast readies the bonus, and the next damaging cast spends it.
- Ward and Remedy can charge the meter. At full charge they preserve it, including an Echo that repeats a support spell.
- Failed casts preserve progress and readiness. Charge has no timeout, and resets on death, hero reset or cleanup.
- Melee hits and automatic passive attacks do not charge or consume the meter.
- Echo consumes readiness only when repeating a damaging spell. Its normal power multiplier still applies.
- Light Broken is an ordinary area attack for 28 +10 damage per rank and needs no charge. White Hand is a wide blast without a separate color bonus.
- Damaging staff hits restore 2 Spirit, plus Wisdom of the Istari and Treachery where applicable. Reserve regeneration grants 1 Spirit every second.
- Lore of Colours adds 15 percentage points per rank to empowered damage. Ring-lore restores 2 Spirit per rank when it is spent. Head of the White Council restores another 6 Spirit.
- Robes of Many Colours uses the number of charged segments. Wisdom of the Istari adds 1 Spirit per rank to damaging staff hits and retains its stat-based regeneration bonus.

The first responsiveness pass removes the half-damage melee penalty and uses separate one-tick attack descriptors for the playable hero, preserving the boss's original assets.
New characters start with 18 Strength and 32 Accuracy. Existing saves keep their stored stats.
Quick spells have a 12-tick cooldown, ordinary spells 30, long spells 90 and White Hand 180, at 30 ticks per second.
Voice, Flame, Blasting Fire, Ward, Binding and Remedy cost 18, 18, 36, 30, 24 and 32 Spirit.
Flame's first rank is available at character level 1.
The damage and resource balance is listed in [Saruman balance](saruman-balance.md).
These changes have host regression coverage and still need normal-control playtesting for timing, hit detection and balance.
Instant damage spells show their impact at release rather than waiting for a decorative bolt to arrive.
Voice's stun markers, healing cues and the ultimate's lightning also begin immediately, with shorter cosmetic follow-up delays.
The skill descriptions distinguish self-protection, self-healing, nearest-target attacks and area control.
Echo and Curse descriptions use the actual formulas of 40% +10% per rank and 20% +5% per rank respectively.

**Builds:** Curunir | White Hand | Sharkey.

| Page 1 actives | Page 2 actives |
| --- | --- |
| Voice of Saruman, Flame of Orthanc, Blasting Fire, Many-Coloured Ward, Word of Binding, Orthanc's Remedy | Echo of Power, Furnaces of Isengard, Sharkey's Curse, Light Broken, Sharkey's Spite, Power of the White Hand |

Page 1 passives: Robes of Many Colours, Keeper of Orthanc, Wisdom of the Istari, Kindling, Wildfire,
Staff of Orthanc, Honeyed Words, Voice of Command, Treachery.

Page 2 passives: Lore of Colours, Ring-lore, Head of the White Council, Fires of Industry, Machine of War,
Powder of Orthanc, Cutpurse, Spite, Parting Shot.

Code: [`saruman.c`](../hacks/patches/ringwraith/saruman.c).
Both pages use 30 custom icons matched to the skill subjects. See the [art mapping and rebuild instructions](../hacks/content/icons/saruman/README.md).

---

## Witch-king

**Idea:** the Lord of the Nazgul as a sorcerer-king. His crown rebounds blows, terror walks with him, and on page 2 no living man may kill him. A PC-only hero (hero id 10) built from the WitchKing enemy's own sprites, in the Servants of Sauron group of the hero select.

**Mechanic: The Black Captain's dread.** Iron Crown turns blows back on the striker, Lord of Terror makes foes cower every second, No Living Man cheats death.

| Page 1 actives | Page 2 actives |
| --- | --- |
| Morgul Shard, Black Breath, Cry of the Nazgul, Iron Crown, Lord of Terror, Undying Malice | Mace of Angmar, Host of Angmar, Morgul Sorcery, Endless Night, No Living Man |

Kit: [`witchking.json`](../hacks/content/heroes/witchking.json). Skill sprites: [`witchking_fx_art.py`](../hacks/tools/witchking_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Orc Captain

**Idea:** a Mordor slave-driver who puts foes where he wants them: a whip along his facing, a kick, a bellow that makes them turn on him. A PC-only hero (hero id 11) built from the OrcCaptainArmored enemy's own sprites, in the Servants of Sauron group of the hero select.

**Mechanic: The lash.** Whip lines, a boot that sends foes flying, taunts, a war-banner haste and a cleave that cuts everyone around him.

| Actives |
| --- |
| Whip Crack, Boot, Over Here, Maggots!, Onward!, Second Wind, Leave None Alive |

Passives: Iron Hide, Slaver's Arm, Loud Voice, Spoils of War, Swagger, Scarred, Cornered Rat, Mordor's Whip, Captain of Mordor.

Kit: [`orccaptain.json`](../hacks/content/heroes/orccaptain.json). Skill sprites: [`orccaptain_fx_art.py`](../hacks/tools/orccaptain_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Uruk-hai

**Idea:** a fighting Uruk who cheats death through rebirth charges earned by kills and paid for in blood. A PC-only hero (hero id 12) built from the Uruk enemy's own sprites, in the Servants of Sauron group of the hero select.

**Mechanic: Bred in the Pits.** Rises from the mud when slain. Whelps, devouring the wounded, a mud hide and the White Hand's lifesteal.

| Actives |
| --- |
| Whelps of the Pit, Mud Tomb, Meat's Back on the Menu, Blood for the Pits, Hide of the Pits, The White Hand |

Passives: Fighting Uruk-hai, Iron of Isengard, Run Day and Night, Bone Pile, Born of Mud, Spawning Pits, Pit-Brood, Sun-Scorned, Deeper Pits.

Kit: [`urukhai.json`](../hacks/content/heroes/urukhai.json). Skill sprites: [`urukhai_fx_art.py`](../hacks/tools/urukhai_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Guard of Gondor

**Idea:** a Citadel guard who brings the city with him: allies, a wall of shields, the horn, beacons, and the Eagles at the end. A PC-only hero (hero id 13) built from the GondorSoldier enemy's own sprites, in the Free Peoples group of the hero select.

**Mechanic: The city's defence.** Calls guards to his side, a shield wall, the Horn of Gondor, a Swan Knight charge and the Eagles.

| Actives |
| --- |
| Call the Guard, Shield Wall!, Horn of Gondor, Charge of Dol Amroth, Light the Beacons, The Eagles Are Coming |

Passives: Guard of the Citadel, Swordsman of Gondor, Shield of the White Tree, Veterans of Osgiliath, Stone of Minas Tirith, Faramir's Rangers, Hold the Line, Men of the West, Captain's Command.

Kit: [`guardofgondor.json`](../hacks/content/heroes/guardofgondor.json). Skill sprites: [`gondor_fx_art.py`](../hacks/tools/gondor_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Denethor

**Idea:** the Steward burning himself out: despair feeds his spells, the palantir stuns, the pyre ends it. A PC-only hero (hero id 14) built from the Denethor enemy's own sprites, in the Free Peoples group of the hero select.

**Mechanic: Fire and despair.** Pays health for spell power, sets foes burning, the White Tower turns arrows, madness trades defence for damage.

| Actives |
| --- |
| Pyre-Flame, The Palantir, Despair, Lord of the White Tower, Madness, Pyre of the Stewards |

Passives: Steward of Gondor, Lore of Numenor, Proud Lineage, Long Sight, Iron Will, Bitter Grief, Seat of the Steward, Ember Heart, Long Despair.

Kit: [`denethor.json`](../hacks/content/heroes/denethor.json). Skill sprites: [`denethor_spider_fx_art.py`](../hacks/tools/denethor_spider_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Mouth of Sauron

**Idea:** Sauron's herald, who fights with lies: charmed foes, marked foes, a mithril shirt that breaks their will, and the Eye. A PC-only hero (hero id 15) built from the MouthOfSauron enemy's own sprites, in the Servants of Sauron group of the hero select.

**Mechanic: Treachery.** Turns a foe against its friends, curses foes to take more damage, rides them down; the Eye calls fire on six.

| Actives |
| --- |
| The Lie, Black Speech, The Mithril Coat, Ride Them Down, Tribute, The Eye Is Upon You |

Passives: Lieutenant of Barad-dur, Sorcerer, Forgotten Name, Black Steed, Cruel Mockery, Grinning Skull, Herald's Due, Dark Tidings, Voice of the Tower.

Kit: [`mouthofsauron.json`](../hacks/content/heroes/mouthofsauron.json). Skill sprites: [`mouthofsauron_fx_art.py`](../hacks/tools/mouthofsauron_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Army of the Dead

**Idea:** the Oathbreakers as one hero: fear, intangibility, lingering cold and a stampede of the dead. A PC-only hero (hero id 16) built from the Ghost enemy's own sprites, in the Free Peoples group of the hero select.

**Mechanic: Terror and the grave.** Foes flee in terror, he passes through them as mist, grave fog and soul harvest; the grey host sweeps the field.

| Actives |
| --- |
| Terror of the Dead, Pass Through, Grave Mist, Soul Harvest, Grasp of the Dead, The Dead Sweep the Field |

Passives: Oathbreakers, Grey Host, Cold Blades, Already Dead, Unquiet, Hollow Voices, Debt of Souls, Isildur's Curse, Dunharrow.

Kit: [`armyofthedead.json`](../hacks/content/heroes/armyofthedead.json). Skill sprites: [`armyofthedead_fx_art.py`](../hacks/tools/armyofthedead_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Warg

**Idea:** a wolf of the Misty Mountains that hunts from foe to foe and grows wilder with every bite. A PC-only hero (hero id 17) built from the Warg enemy's own sprites, in the Servants of Sauron group of the hero select.

**Mechanic: Frenzy.** Every bite stacks Frenzy (a HUD meter). Leaps, howls that weaken, bleeding bites and a throat-bite finisher.

| Actives |
| --- |
| Pounce, Howl, Rend, Go for the Throat, Feeding Frenzy, Hunt Down |

Passives: Thick Pelt, Long Stride, Sharp Fangs, Blood Scent, Hunter's Patience, Pack Lore, Savage, Howling Wild, Alpha.

Kit: [`warg.json`](../hacks/content/heroes/warg.json). Skill sprites: [`warg_fx_art.py`](../hacks/tools/warg_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Spider

**Idea:** a spawn of Ungoliant that fights from a distance and makes foes come to her. A PC-only hero (hero id 18) built from the Spider enemy's own sprites, in the Servants of Sauron group of the hero select.

**Mechanic: Web traps.** Webs that hold foes, stacking venom that paralyzes, reels foes in, cocoons them; Shelob's Lair rings her in webs.

| Actives |
| --- |
| Web Trap, Venom Bite, Reel In, Cocoon, Skitter, Shelob's Lair |

Passives: Chitin, Eight Eyes, Patient Hunter, Venom Sacs, Skittering, Feast, Old Hunger, Lair Born, Brood Mother.

Kit: [`spider.json`](../hacks/content/heroes/spider.json). Skill sprites: [`denethor_spider_fx_art.py`](../hacks/tools/denethor_spider_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Corsair

**Idea:** a raider of Umbar: dirty fighting, gunpowder and loot. A PC-only hero (hero id 19) built from the Corsair enemy's own sprites, in the Servants of Sauron group of the hero select.

**Mechanic: Plunder.** Kills drop loot. A grappling hook, cutlass flurry, blinding smoke powder, powder kegs and a broadside of cannon.

| Actives |
| --- |
| Grappling Hook, Cutlass Flurry, Smoke Powder, Plunder, Powder Keg, Black Sails |

Passives: Sea Legs, Cutthroat, Salt-Tough, Treasure Nose, Quick Hands, Grog, Dirty Fighter, Fear of the Lash, Captain of Umbar.

Kit: [`corsair.json`](../hacks/content/heroes/corsair.json). Skill sprites: [`corsair_fx_art.py`](../hacks/tools/corsair_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---

## Uruk Crossbowman

**Idea:** an Isengard crossbowman rewarded for holding his ground: shields, aim that builds while he stands still, and volleys. A PC-only hero (hero id 20) built from the UrukCrossbowman enemy's own sprites, in the Servants of Sauron group of the hero select.

**Mechanic: The marksman.** His basic attack shoots bolts. Piercing bolts, volleys, a pavise to stand behind, fire that spreads, steady aim, a rain of bolts.

| Actives |
| --- |
| Heavy Bolt, Volley, Pavise, Fire Bolt, Steady Aim, Rain of Bolts |

Passives: Steady Hands, Heavy Draw, Isengard Plate, Quiver, Run Day and Night, Picked Off, Cold Eye, Sun-Scorned, Fighting Uruk-hai.

Kit: [`urukcrossbowman.json`](../hacks/content/heroes/urukcrossbowman.json). Skill sprites: [`urukcrossbowman_fx_art.py`](../hacks/tools/urukcrossbowman_fx_art.py).

Status: **headless checks pass; manual pass pending.**

---
## Adding or changing a hero

Start with [hero authoring](wip-docs/howto-heroes-skills.md) and [hacking recipes](wip-docs/hacking-recipes.md),
then see [engine changes](engine-changes.md) for the hooks a kit can use. New heroes are registered in
[`hacks/content/heroes.json`](../hacks/content/heroes.json), which maps each hero id to the stock hero whose model,
equipment and skill tree art it borrows.

For a PC-only hero built from an enemy, `hacks/tools/new_hero.py` does the registration, a skill spec in
`hacks/content/heroes/<hero>.json` replaces a hand-written kit, and the fast tools check and play the result in
seconds. The [hero authoring guide](wip-docs/howto-heroes-skills.md) has the loop.
