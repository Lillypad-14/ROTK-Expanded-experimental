#include "player.h"
#define OFF(f) ((unsigned)&((Player*)0)->f)
int a_combatFlags = OFF(combatFlags);
int a_runes = OFF(runes);
int a_selected = OFF(selectedActiveSkill);
int a_status = OFF(statusFlags);
int a_f1a4 = OFF(field_0x1a4);
int a_asf = OFF(activeSkillStateFlags);
int a_stats = OFF(stats);
int a_casting=OFF(castingActiveSkill);
int a_option=OFF(optionFlags);
int a_med=OFF(meditativeTimer);
