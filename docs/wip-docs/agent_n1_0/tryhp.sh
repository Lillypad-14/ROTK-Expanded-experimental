#!/bin/bash
# usage: tryhp.sh <<< body
cat > player_computeMaxHp.c <<E2
#define player_computeMaxHp hdr_dummy
#include "player.h"
#undef player_computeMaxHp

/**
 * Max HP from the base stats: 4 * health + courage + strength.
 *
 * @romaddress 0x08027348
 */
s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
$(cat)
}
E2
bash m.sh player_computeMaxHp | head -${N:-2} | cut -c1-90
