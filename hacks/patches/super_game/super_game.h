#pragma once

#include "types.h"

// The super game (PC edition): a "The Two Towers" row on the main menu that hands the window to The
// Two Towers. The PC host runs it and brings this game back when it quits.

// The main menu's rows: the stock six, and The Two Towers when the host can start it.
u32 superGame_mainMenuRowCount(void);
// The main menu row for The Two Towers was confirmed.
void superGame_mainMenuConfirm(void);

#define SUPER_GAME_MAIN_MENU_ROW 6
