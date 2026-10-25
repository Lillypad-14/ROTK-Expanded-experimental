from copy import deepcopy

from gauntlet_run import OVER_FIELDS, ROOM_FIELDS, parse_log, report


def clean_run():
    room = dict.fromkeys(ROOM_FIELDS, 0)
    room.update(room=1, bssGuardIntact=1)
    over = dict.fromkeys(OVER_FIELDS, 0)
    over.update(room=2, bssGuardIntact=1)
    return dict(booted=True, rooms=[room], over=over, done=True, stuck=[], seconds=1)


def test_completed_run_still_rejects_memory_and_spawn_failures():
    clean = clean_run()
    assert report(2, clean)
    for field, value in (("allocFailures", 1), ("landFails", 1), ("bssGuardIntact", 0)):
        failed = deepcopy(clean)
        failed["rooms"][0][field] = value
        assert not report(2, failed)
    for field, value in (("allocFailures", 1), ("bssGuardIntact", 0)):
        failed = deepcopy(clean)
        failed["over"][field] = value
        assert not report(2, failed)


def test_emulator_faults_fail_even_when_a_run_claims_completion():
    failed = clean_run()
    failed["errors"] = ["Illegal opcode"]
    assert not report(2, failed)
    assert parse_log("[WARN] GBA BIOS: Stub software interrupt: FF")["errors"]
    assert parse_log("[WARN] GBA: Illegal opcode: 000038d2")["errors"]


def test_entry_collision_is_a_failure():
    result = clean_run()
    result["entries"] = parse_log("GAUNTLET_ENTRY 5 2 8 524 388 1")["entries"]
    assert result["entries"][0]["roomType"] == 5
    assert report(2, result)
    result["entries"][0]["clear"] = 0
    assert not report(2, result)


def test_variant_telemetry_keeps_modifier_and_reward():
    result = parse_log("GAUNTLET_VARIANT 2 1\nGAUNTLET_VARIANT 0 2")
    assert result["variants"] == [
        {"modifier": 2, "rarity": 1},
        {"modifier": 0, "rarity": 2},
    ]


def test_water_backdrop_state_is_recorded():
    result = parse_log("GAUNTLET_BACKDROP 39 6 7183 28")
    assert result["backdrops"] == [
        {"mission": 39, "effect": 6, "control": 0x1C0F, "frame": 28}
    ]
