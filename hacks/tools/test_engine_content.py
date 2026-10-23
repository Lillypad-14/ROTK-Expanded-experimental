"""Check registry input and linked-content validation against invalid expansions."""

from copy import deepcopy
import json
from pathlib import Path
import struct
import tempfile

from hero_registry import generate
from validate_abilities import LinkedContent, SCHEMA, validate


def write_bytes(content: LinkedContent, address: int, replacement: bytes) -> None:
    for index, (start, data) in enumerate(content.regions):
        offset = address - start
        if 0 <= offset and offset + len(replacement) <= len(data):
            content.regions[index] = (
                start,
                data[:offset] + replacement + data[offset + len(replacement) :],
            )
            return
    raise AssertionError(f"No linked section contains {address:#x}")


def rejects(content: LinkedContent, message: str) -> None:
    try:
        validate(content)
    except ValueError as error:
        assert message in str(error), error
    else:
        raise AssertionError(f"Validator accepted invalid content: {message}")


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    content = LinkedContent(
        (root / "build/hack.elf").read_bytes(), (root / "rotk.gba").read_bytes()
    )
    assert validate(content) > 0
    start = content.symbols["__modcode_bss_start"][0]
    end = content.symbols["__modcode_bss_end"][0]
    assert 0x0203F5A0 <= start <= end <= 0x02040000
    assert start == 0x02040000 - ((end - start + 15) & ~15)
    address, size = content.symbols["SkillExpansionValidationSchema"]
    schema = dict(
        zip(SCHEMA, struct.unpack(f"<{size // 4}I", content.read(address, size)))
    )
    legacy = content.symbols["LegacyActiveIds"][0]
    bad = deepcopy(content)
    write_bytes(bad, legacy + 2, content.read(legacy, 2))
    rejects(bad, "duplicate permanent active IDs")
    bad = deepcopy(content)
    write_bytes(bad, legacy, b"\0\0")
    rejects(bad, "missing permanent save ID")

    definitions = content.symbols["SkillExpansionActiveDefs"][0]
    definition = next(
        content.pointer(definitions + index * 4)
        for index in range(schema["heroes"] * schema["new_rows"])
        if content.pointer(definitions + index * 4)
    )
    bad = deepcopy(content)
    write_bytes(bad, definition + schema["def_delivery"], struct.pack("<I", 0x09FFFFFF))
    rejects(bad, "invalid delivery recipe")
    deliveries = content.symbols["SkillDeliveryDefinitions"][0]
    write_bytes(bad, definition + schema["def_delivery"], struct.pack("<I", deliveries))
    write_bytes(bad, definition + schema["def_resolve"], struct.pack("<I", 1))
    rejects(bad, "delivery conflicts with custom resolver")
    write_bytes(bad, definition + schema["def_resolve"], bytes(4))
    write_bytes(bad, definition + schema["def_stock"], bytes(4))
    assert validate(bad) > 0

    policy = 0x09F00000
    bad = deepcopy(content)
    bad.regions.append((policy, bytes((1, 254, 255, 0, 60, 0))))
    write_bytes(bad, definition + schema["def_policy"], struct.pack("<I", policy))
    rejects(bad, "invalid cooldown row")
    bad.regions[-1] = (policy, bytes((1, 255, 255, 0, 60, 0)))
    assert validate(bad) > 0

    timeline = 0x09F00100
    bad = deepcopy(content)
    payload = bytearray(schema["timeline_size"])
    struct.pack_into("<H", payload, schema["timeline_release"], 3600)
    struct.pack_into("<H", payload, schema["timeline_recovery"], 1)
    bad.regions.append((timeline, bytes(payload)))
    write_bytes(bad, definition + schema["def_timeline"], struct.pack("<I", timeline))
    rejects(bad, "invalid timeline bounds")
    payload[schema["timeline_recovery"]] = 0
    payload[schema["timeline_count"]] = 1
    bad.regions[-1] = (timeline, bytes(payload))
    rejects(bad, "missing timeline events")
    events = timeline + 0x100
    struct.pack_into("<I", payload, 0, events)
    bad.regions[-1] = (timeline, bytes(payload))
    event = bytearray(schema["event_size"])
    event[schema["event_anchor"]] = 3
    bad.regions.append((events, bytes(event)))
    rejects(bad, "invalid timeline cue")
    event[schema["event_anchor"]] = 0
    bad.regions[-1] = (events, bytes(event))
    assert validate(bad) > 0

    heroes = json.loads((root / "hacks/content/heroes.json").read_text())
    with tempfile.TemporaryDirectory(prefix="rotk-hero-tests-") as directory:
        scratch = Path(directory)
        (scratch / "hacks/content").mkdir(parents=True)
        (scratch / "build").mkdir()
        manifest = scratch / "hacks/content/heroes.json"
        manifest.write_text(json.dumps(heroes))
        generate(scratch)
        assert "HeroEquipmentIds:" in (scratch / "build/hero_tables.asm").read_text()
        heroes[8]["id"] = 12
        manifest.write_text(json.dumps(heroes))
        try:
            generate(scratch)
        except ValueError:
            pass
        else:
            raise AssertionError("Registry accepted nonconsecutive hero IDs")
    print("PASS: hero manifest, stable IDs, cast policies and native heap headroom")


if __name__ == "__main__":
    main()
