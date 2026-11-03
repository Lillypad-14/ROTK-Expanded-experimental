#pragma once

// The bonus-map hero select (PC edition): a screen of hero cards by group, with the highlighted
// hero's portrait, stats and description. Picking an empty bonus-map save slot opens it. A
// returns to the save menu, which creates the character as its own hero select would.

#include "player.h"
#include "types.h"

typedef struct HeroMenuInfo
{
    // One line or two about the hero.
    const char *description;
    // Index into HeroGroupNames.
    u8 group;
    // Dialog portrait asset shown beside the cards.
    const u8 *portrait;
    // Starting stats shown as bars.
    const HeroBaseStats *stats;
} HeroMenuInfo;

/** Per hero, from hacks/content/heroes.json. */
extern const HeroMenuInfo HeroMenuInfos[];

void heroSelect_sceneEnter(void);
void heroSelect_sceneTick(void);
void heroSelect_sceneExit(void);

/** TRUE while the save menu's own hero select runs for a pick made on this screen. */
bool32 heroSelect_committing(void);

/** Open the screen for the save menu's selected slot. */
void heroSelect_open(void);

/** Called at the end of the save menu's enter: create the hero picked on this screen, if any. */
void heroSelect_resumeSaveMenu(void);
