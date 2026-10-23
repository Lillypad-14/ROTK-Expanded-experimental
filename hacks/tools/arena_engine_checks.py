"""Generate assertions against the production engine running inside mGBA."""

import json
from pathlib import Path


def script(elf: Path) -> str:
    lines = [
        "set pagination off",
        "set confirm off",
        "set remote noack-packet off",
        "set remote interrupt-on-connect on",
        "set remote binary-download-packet off",
        "set tcp auto-retry on",
        "set tcp connect-timeout 30",
        f"file {json.dumps(elf.as_posix())}",
        "target remote 127.0.0.1:2345",
        "break testArena_checkpoint",
        "continue",
    ]

    def check(expression: str, name: str) -> None:
        lines.extend(
            [
                f"if !({expression})",
                f'printf "FAIL {name}\\n"',
                "print *g_TestArena",
                "quit 1",
                "end",
                f'printf "ARENA_CHECK {name}\\n"',
            ]
        )

    def advance(ticks: int) -> None:
        lines.extend(
            [
                f"set $endTick = g_TestArena->ticks + {ticks}",
                "condition 1 g_TestArena->ticks >= $endTick",
                "continue",
                "condition 1",
            ]
        )

    def command(
        kind: int,
        argument: int = 0,
        *,
        level: int = 1,
        cosmetic: bool = False,
        expected: bool = True,
    ) -> None:
        lines.extend(
            [
                f"set g_TestArena->command = {kind}",
                f"set g_TestArena->argument = {argument}",
                f"set g_TestArena->level = {level}",
                f"set g_TestArena->cosmetic = {int(cosmetic)}",
                "set g_TestArena->request = g_TestArena->request + 1",
                "continue",
                "if g_TestArena->acknowledged != g_TestArena->request || "
                f"g_TestArena->result != {int(expected)}",
                'printf "FAIL command\\n"',
                "print *g_TestArena",
                "quit 1",
                "end",
            ]
        )

    def effect(kind: int) -> None:
        lines.extend(
            [
                "set $found = (HackLibTimedEffect*)0",
                "set $pool = 'timed_effect.c'::effects",
                "set $index = 0",
                "while $pool != 0 && $index < 12",
                f"if $pool[$index].definition != 0 && $pool[$index].definition->id == {kind}",
                "set $found = &$pool[$index]",
                "end",
                "set $index = $index + 1",
                "end",
            ]
        )

    check("g_TestArena->ready == 1", "cold_boot")
    lines.extend(
        [
            "set $events = 0",
            "set $eventDamage = 0",
            "set $eventKills = 0",
            "set $badPeriodicTags = 0",
            "set $periodic = 0",
            "break *hackLib_dispatchCombatEvent",
            "commands 2",
            "silent",
            "set $event = (HackLibCombatEvent*)$r0",
            "set $events = $events + 1",
            "set $eventDamage = $eventDamage + $event->result.damageDealt",
            "set $eventKills = $eventKills + $event->result.killed",
            "if $event->source.kind == 5",
            "set $periodic = $periodic + 1",
            "if !$event->source.elementsKnown || ($event->source.elements != 1 && $event->source.elements != 2)",
            "set $badPeriodicTags = $badPeriodicTags + 1",
            "end",
            "end",
            "continue",
            "end",
        ]
    )

    command(1)
    command(2, 5, level=5, cosmetic=True)
    advance(12)
    check(
        "g_TestArena->hits == 0 && g_TestArena->targetHp[0] == 30000 && 'timed_effect.c'::effects == 0",
        "cosmetic_has_no_damage_or_status",
    )

    command(1)
    command(2, 0)
    advance(10)
    lines.append("set $levelOneDamage = g_TestArena->damage")
    command(1)
    command(2, 0, level=5)
    advance(10)
    check(
        "$levelOneDamage > 0 && g_TestArena->damage > $levelOneDamage",
        "level_damage_scaling",
    )

    command(1)
    command(2, 5)
    command(2, 5, expected=False)
    advance(10)
    effect(0xB01)
    check(
        "$found != 0 && $found->stacks == 1 && $found->magnitude == 2",
        "poison_application_and_cooldown",
    )
    lines.append("set $remaining = $found->remaining")
    advance(20)
    command(2, 5)
    advance(10)
    effect(0xB01)
    check(
        "$found != 0 && $found->stacks == 2 && $found->remaining > $remaining - 30",
        "poison_stack_and_refresh",
    )
    advance(100)
    check("'timed_effect.c'::effects == 0", "poison_expires")

    command(1)
    lines.extend(["set $events = 0", "set $eventDamage = 0", "set $eventKills = 0"])
    command(2, 6)
    advance(12)
    effect(0xB02)
    check("$found != 0 && $found->stacks == 1", "burn_application")
    advance(20)
    command(2, 6)
    advance(12)
    effect(0xB02)
    check(
        "$found != 0 && $found->stacks == 1 && $found->remaining > 65",
        "burn_refresh_without_stacking",
    )
    advance(100)
    check(
        "'timed_effect.c'::effects == 0 && $periodic > 0 && $badPeriodicTags == 0",
        "periodic_elements_and_expiry",
    )
    check(
        "$eventDamage == g_TestArena->damage && $eventKills == g_TestArena->kills",
        "combat_event_damage_and_kill_totals",
    )
    check(
        "'combat_source.c'::currentSource == 0 && 'combat_event.c'::dispatchDepth == 0",
        "combat_context_restored",
    )

    command(1)
    command(2, 7)
    advance(2)
    effect(0xB03)
    check(
        "$found != 0 && $found->definition->movementPercent == 50", "slow_application"
    )
    command(5)
    advance(3)
    check(
        "'timed_effect.c'::effects == 0 && g_TestArena->targets[0] == 0",
        "clear_attached_slow",
    )

    command(1)
    lines.append("set g_TestArena->targets[0]->currentHp = 1")
    command(2, 0, level=5)
    advance(10)
    check(
        "g_TestArena->kills == 1 && g_TestArena->damage == 1",
        "overkill_clamped_and_single_kill",
    )
    command(2, 0)
    advance(10)
    check("g_TestArena->kills == 1", "dead_target_no_duplicate_kill")

    command(1)
    lines.extend(
        [
            "set $failures = 'memory_diagnostics.c'::stats.failures",
            "set $invalid = 'memory_diagnostics.c'::stats.invalidFrees",
        ]
    )
    for _ in range(12):
        command(2, 5)
        advance(10)
        command(5)
        advance(3)
        command(1)
    check(
        "'timed_effect.c'::effects == 0 && 'memory_diagnostics.c'::stats.failures == $failures && "
        "'memory_diagnostics.c'::stats.invalidFrees == $invalid",
        "repeated_status_spawn_clear",
    )

    lines.extend(
        [
            'printf "ARENA_JSON {\\"ready\\":%u,\\"ticks\\":%u,\\"periodicEvents\\":%u}\\n", '
            "g_TestArena->ready, g_TestArena->ticks, $periodic",
            "disconnect",
            "quit",
        ]
    )
    return "\n".join(lines) + "\n"
