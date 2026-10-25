#pragma once

#include "patches/gauntlet/gauntlet_boons.h"

void gauntletBoonEffects_reset(void);
void gauntletBoonEffects_roomStart(void);
void gauntletBoonEffects_tick(u8 playerIndex, Actor *owner);
void gauntletBoonEffects_event(u8 playerIndex, const HackLibCombatEvent *event);
s32 gauntletBoonEffects_damage(u8 playerIndex, Actor *victim, s32 damage);
void gauntletBoonEffects_hp(u8 playerIndex, s32 *amount, bool *flinch);
void gauntletBoons_castStarted(u8 playerIndex, u8 row, s32 before, s32 paid);
void gauntletBoons_castResolved(u8 playerIndex, u8 row, bool success);
u16 gauntletBoons_radius(Actor *owner, u16 radius);
void gauntletBoons_rechargeSecondWind(void);
void gauntletBoons_ignite(Actor *target);

void gauntletBoonEffects_statsRebuilt(void);

void gauntletBoonEffects_roomEnd(void);

/** Schedule bounded secondary damage outside the native hit callback. */
bool gauntletBoonEffects_queue(Actor *target, s32 damage, u8 kind, u8 ticks);
