# Saruman skill artwork

Thirty icons selected by subject from the two supplied Saruman sheets.
The mapping deliberately uses artwork from either sheet wherever it best matches the skill.

Rebuild the checked-in native C icons and a labeled preview:

```sh
uv run --with numpy python hacks/tools/saruman_icons.py
```

To repeat the original sheet extraction, add `--import-sheets`.
Extraction requires the stock icon frame from your own ROM under `carved-local/`.
Edit `manifest.json` to change a selection, or edit the individual 32x24 indexed PNGs to refine an icon.
The importer maps art to the shared game palette, so source blue/grey tones can become green.
It restores the stock frame, excludes the sheet captions and rank boxes, and keeps palette index zero transparent.
Generated code lives in `hacks/patches/ringwraith/saruman_icons.c` and `.h`.
The local preview is `build/saruman_icons_preview.png`.

Slots below are one-based, left to right, top to bottom within each 8x3 sheet.

| Skill | Sheet | Row, column | Selected subject |
| --- | --- | --- | --- |
| [Staff Of Orthanc](png/staff_of_orthanc.png) | [Saruman.png](source/Saruman.png) | 1, 1 | Glowing staff |
| [Honeyed Words](png/honeyed_words.png) | [Saruman.png](source/Saruman.png) | 1, 2 | Saruman portrait |
| [Keeper Of Orthanc](png/keeper_of_orthanc.png) | [Saruman.png](source/Saruman.png) | 1, 4 | Orthanc tower |
| [Power Of The White Hand](png/power_of_the_white_hand.png) | [Saruman.png](source/Saruman.png) | 1, 5 | White Hand emblem |
| [Parting Shot](png/parting_shot.png) | [Saruman.png](source/Saruman.png) | 1, 7 | Enemies thrown back |
| [Furnaces Of Isengard](png/furnaces_of_isengard.png) | [Saruman.png](source/Saruman.png) | 2, 1 | Industrial Isengard |
| [Kindling](png/kindling.png) | [Saruman.png](source/Saruman.png) | 2, 2 | Burning tree |
| [Orthancs Remedy](png/orthancs_remedy.png) | [Saruman.png](source/Saruman.png) | 2, 3 | Restorative light around a kneeling wizard |
| [Blasting Fire](png/blasting_fire.png) | [Saruman.png](source/Saruman.png) | 2, 4 | Tower exploding |
| [Sharkeys Spite](png/sharkeys_spite.png) | [Saruman.png](source/Saruman.png) | 2, 5 | Dark explosive orb |
| [Powder Of Orthanc](png/powder_of_orthanc.png) | [Saruman.png](source/Saruman.png) | 2, 6 | Explosive debris |
| [Voice Of Saruman](png/voice_of_saruman.png) | [Saruman.png](source/Saruman.png) | 2, 7 | Commanding wizard |
| [Head Of The White Council](png/head_of_the_white_council.png) | [Saruman.png](source/Saruman.png) | 2, 8 | White-robed leader with companions |
| [Flame Of Orthanc](png/flame_of_orthanc.png) | [Saruman.png](source/Saruman.png) | 3, 6 | Burning Orthanc |
| [Treachery](png/treachery.png) | [Saruman.png](source/Saruman.png) | 3, 7 | Watching sinister eye |
| [Robes Of Many Colours](png/robes_of_many_colours.png) | [Saruman.png](source/Saruman.png) | 3, 8 | Robed wizard |
| [Wisdom Of The Istari](png/wisdom_of_the_istari.png) | [Saruman2.png](source/Saruman2.png) | 1, 2 | Wizard studying his staff |
| [Echo Of Power](png/echo_of_power.png) | [Saruman2.png](source/Saruman2.png) | 1, 3 | Repeating spiral of power |
| [Wildfire](png/wildfire.png) | [Saruman2.png](source/Saruman2.png) | 1, 7 | Flaming hand |
| [Machine Of War](png/machine_of_war.png) | [Saruman2.png](source/Saruman2.png) | 1, 8 | Army of Isengard |
| [Many Coloured Ward](png/many_coloured_ward.png) | [Saruman2.png](source/Saruman2.png) | 2, 1 | Protective ring around the wizard |
| [Cutpurse](png/cutpurse.png) | [Saruman2.png](source/Saruman2.png) | 2, 4 | Orc raider |
| [Sharkeys Curse](png/sharkeys_curse.png) | [Saruman2.png](source/Saruman2.png) | 2, 5 | Cursed shadow |
| [Spite](png/spite.png) | [Saruman2.png](source/Saruman2.png) | 2, 7 | Bitter scarred face |
| [Lore Of Colours](png/lore_of_colours.png) | [Saruman2.png](source/Saruman2.png) | 2, 8 | Charged magical crystal |
| [Light Broken](png/light_broken.png) | [Saruman2.png](source/Saruman2.png) | 3, 1 | Crystal bursting with light |
| [Voice Of Command](png/voice_of_command.png) | [Saruman2.png](source/Saruman2.png) | 3, 4 | Raised staff commanding power |
| [Fires Of Industry](png/fires_of_industry.png) | [Saruman2.png](source/Saruman2.png) | 3, 6 | Isengard towers |
| [Word Of Binding](png/word_of_binding.png) | [Saruman2.png](source/Saruman2.png) | 3, 7 | Hand controlling magical rings |
| [Ring Lore](png/ring_lore.png) | [Saruman2.png](source/Saruman2.png) | 3, 8 | Seeing-stone eye |
