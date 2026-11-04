// Denethor: a playable enemy body made by hacks/tools/new_hero.py. Its animations are "body" in
// hacks/content/heroes.json (hacks/tools/hero_clips.py). Edit the stats freely.
#include "patches/ringwraith/hero_registry.h"

#if PC_EDITION

const HeroBaseStats DenethorBaseStats = {
    .courage = 30,
    .defense = 20,
    .health = 22,
    .accuracy = 26,
    .strength = 24,
};

#endif
