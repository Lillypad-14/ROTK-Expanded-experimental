"""Run the actual patch C with host-side player and SRAM stubs."""

from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
STUBS = r'''
#pragma once
#include "types.h"
#define ACTIVE_SKILL_MAX_LEVEL 5
#define STAT_ACTIVE_SKILL_LEVEL_BONUS 0
#define STAT_MAX_HP 1
#define STAT_MAX_SPIRIT 2
#define PLAYER_STATUS_FLAG_UNSPENT_POINTS 1
#define PLAYERHUD_SKILL_PANEL_IDLE 0
#define PLAYERHUD_SKILL_PANEL_DRAW_UNSPENT_POINTS 1
#define PLAYERHUD_FLAG_XP_DIRTY 1
#define DEBUG_CHEATS_TAG 0
typedef struct ActiveSkillValue { s16 base; s16 perLevel[5]; } ActiveSkillValue;
typedef struct ActiveSkill { ActiveSkillValue values[2]; } ActiveSkill;
typedef struct Player {
    u8 level, attributePoints, skillPoints;
    u32 experience;
    s16 currentHp, currentSpirit, stats[3];
    struct { u32 p; } statusFlags;
    ActiveSkill activeSkills[12];
} Player;
typedef struct Actor { s16 currentHp; } Actor;
struct DebugCheatsState {
    u8 maxLevel, skillPoints, attributePoints, godMode, infiniteSpirit, oneHitKills, tag;
};
typedef struct TestHud {
    u8 skillPanelState;
    struct { u32 p; } flags;
} TestHud;
extern Player testPlayers[2];
extern TestHud g_PlayerHuds[2];
extern struct DebugCheatsState g_DebugCheatsState;
#define PLAYER(p) testPlayers[p]
#define PLAYER_ACTIVE_SKILL(p, r) (PLAYER(p).activeSkills[r])
#define PLAYER_STAT(p, s) (PLAYER(p).stats[s])
#define ACTIVE_PLAYER_INDEX 0
#define GLOBAL_MOD_OPTIONS_READ(field, dst) ((void)(dst))
'''
HARNESS = r'''
#include "stubs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
Player testPlayers[2];
TestHud g_PlayerHuds[2];
s16 skillExpansion_activeGetLeveledValue(u8 p, u8 row, u8 value, u8 level);
void debugCheats_playerTick(u8 p);
int main(void) {
    static const s16 sums[] = {0, 1, 3, 7, 15, 31};
    unsigned p, row, value, level, bonus, i, old, initial;
    for (p = 0; p < 2; p++) {
        for (row = 0; row < 12; row++) {
            for (value = 0; value < 2; value++) {
                ActiveSkillValue *curve = &PLAYER_ACTIVE_SKILL(p, row).values[value];
                s16 sign = value == 0 ? 1 : -1;
                curve->base = 100 + row;
                for (i = 0; i < 5; i++) curve->perLevel[i] = sign * (1 << i);
                for (bonus = 0; bonus < 2; bonus++) {
                    PLAYER(p).stats[STAT_ACTIVE_SKILL_LEVEL_BONUS] = bonus;
                    for (level = 0; level < 256; level++) {
                        unsigned capped = level > 5 ? 5 : level;
                        s16 expected = curve->base + sign * sums[capped];
                        if (bonus) expected += sign * (1 << (capped < 5 ? capped : 4));
                        assert(skillExpansion_activeGetLeveledValue(p, row, value, level) == expected);
                    }
                }
            }
        }
    }
    memset(&g_DebugCheatsState, 0, sizeof(g_DebugCheatsState));
    g_DebugCheatsState.maxLevel = 1;
    for (p = 0; p < 2; p++) {
        for (old = 0; old <= 50; old++) {
            for (initial = 0; initial <= 255; initial++) {
                unsigned expected = initial;
                unsigned attributes = initial + (50 - old) * 5;
                memset(testPlayers, 0, sizeof(testPlayers));
                memset(g_PlayerHuds, 0, sizeof(g_PlayerHuds));
                PLAYER(p).level = old;
                PLAYER(p).skillPoints = initial;
                PLAYER(p).attributePoints = initial;
                for (level = old + 1; level <= 50; level++) {
                    expected++;
                    if (HACK_skill_expansion && level >= 20) expected++;
                }
                if (expected > 255) expected = 255;
                if (attributes > 255) attributes = 255;
                debugCheats_playerTick(p);
                assert(PLAYER(p).level == 50);
                assert(PLAYER(p).skillPoints == expected);
                assert(PLAYER(p).attributePoints == attributes);
                assert(PLAYER(1 - p).level == 0);
                assert(PLAYER(1 - p).skillPoints == 0);
                debugCheats_playerTick(p);
                assert(PLAYER(p).skillPoints == expected);
            }
        }
    }
    puts("PASS: curve boundaries, both players, signed curves, bonuses, level grants, saturation, repeat ticks");
    return 0;
}
'''

compiler = shutil.which("clang") or shutil.which("cc")
assert compiler, "A native C compiler is required"
with tempfile.TemporaryDirectory(prefix="rotk-skill-tests-") as directory:
    temporary = Path(directory)
    (temporary / "stubs.h").write_text(STUBS)
    for header in (
        "player.h", "skill.h", "stats.h", "variables.h",
        "patches/debug_cheats/debug_cheats.h", "patches/mod_save/save_sram.h",
    ):
        path = temporary / header
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "stubs.h"\n')
    harness = temporary / "test.c"
    harness.write_text(HARNESS)
    for expansion in (0, 1):
        executable = temporary / f"test-{expansion}.exe"
        subprocess.run([
            compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
            f"-I{temporary}", f"-I{ROOT / 'include'}",
            f"-DHACK_skill_expansion={expansion}", "-DHACK_mod_options=0",
            str(ROOT / "hacks/patches/skill_expansion/skill_expansion_values.c"),
            str(ROOT / "hacks/patches/debug_cheats/debug_cheats.c"),
            str(harness), "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)
