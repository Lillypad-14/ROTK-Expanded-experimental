"""Validate linked skill content using compiler-derived layouts before patching the ROM."""

from pathlib import Path
import struct
import sys

SCHEMA = (
    "heroes main_rows new_rows levels passive_end kit_size kit_active kit_icons kit_levels "
    "kit_passives kit_new kit_new_passives kit_resolvers active_size active_callback "
    "def_size def_row def_stock def_icon def_levels def_resolve def_base main_passives new_passives "
    "def_policy def_id kit_policies kit_ids policy_size policy_cooldown kit_art "
    "def_timeline kit_timelines timeline_size timeline_release timeline_recovery timeline_count "
    "event_size event_effect event_anchor effect_count def_delivery delivery_size"
).split()


class LinkedContent:
    def __init__(self, elf: bytes, base_rom: bytes):
        if elf[:6] != b"\x7fELF\x01\x01":
            raise ValueError("Expected a little-endian ELF32 file")
        self.base_rom = base_rom
        self.regions: list[tuple[int, bytes]] = []
        self.symbols: dict[str, tuple[int, int]] = {}
        section_offset = struct.unpack_from("<I", elf, 32)[0]
        section_size, section_count = struct.unpack_from("<HH", elf, 46)
        sections = [
            struct.unpack_from("<10I", elf, section_offset + i * section_size)
            for i in range(section_count)
        ]
        for section in sections:
            _, kind, flags, address, offset, size, link, _, _, entry_size = section
            if flags & 2 and kind != 8:
                self.regions.append((address, elf[offset : offset + size]))
            if kind == 2:
                strings = sections[link]
                names = elf[strings[4] : strings[4] + strings[5]]
                for cursor in range(offset, offset + size, entry_size):
                    name, value, length, _, _, index = struct.unpack_from(
                        "<IIIBBH", elf, cursor
                    )
                    if name and index:
                        end = names.find(b"\0", name)
                        self.symbols[names[name:end].decode()] = (value, length)

    def read(self, address: int, size: int) -> bytes:
        for start, data in self.regions:
            if start <= address and address + size <= start + len(data):
                return data[address - start : address - start + size]
        offset = address - 0x08000000
        if 0 <= offset and offset + size <= len(self.base_rom):
            return self.base_rom[offset : offset + size]
        raise ValueError(f"Content pointer outside linked ROM: {address:#x} (+{size})")

    def pointer(self, address: int) -> int:
        return struct.unpack("<I", self.read(address, 4))[0]


def validate(content: LinkedContent) -> int:
    if "SkillExpansionValidationSchema" not in content.symbols:
        return 0
    address, length = content.symbols["SkillExpansionValidationSchema"]
    if length != len(SCHEMA) * 4:
        raise ValueError("Ability validation schema changed; update the validator")
    schema = dict(
        zip(SCHEMA, struct.unpack(f"<{len(SCHEMA)}I", content.read(address, length)))
    )
    checked = 0

    def require(condition: bool, message: str) -> None:
        if not condition:
            raise ValueError(message)

    def curve(pointer: int, label: str) -> None:
        require(pointer != 0, f"{label}: missing required-level curve")
        size = next(
            (
                size
                for address, size in content.symbols.values()
                if address == pointer and size
            ),
            None,
        )
        require(
            size is None or size >= schema["levels"], f"{label}: truncated level curve"
        )
        values = struct.unpack(
            f"{schema['levels']}b", content.read(pointer, schema["levels"])
        )
        require(values[-1] == -1, f"{label}: level curve must end with -1")
        stopped = False
        previous = -1
        for value in values:
            if value == -1:
                stopped = True
            else:
                require(
                    not stopped and previous <= value <= 50,
                    f"{label}: invalid required level {value}",
                )
                previous = value

    def row(pointer: int, resolver: int, label: str) -> None:
        require(pointer != 0, f"{label}: missing active row")
        content.read(pointer, schema["active_size"])
        callback = content.pointer(pointer + schema["active_callback"])
        require(
            callback != 0 or resolver != 0, f"{label}: missing callback or resolver"
        )

    def definition(pointer: int, label: str) -> None:
        nonlocal checked
        content.read(pointer, schema["def_size"])
        get = lambda name: content.pointer(pointer + schema[name])
        delivery = get("def_delivery")
        if delivery:
            start, size = content.symbols["SkillDeliveryDefinitions"]
            require(
                start <= delivery < start + size
                and (delivery - start) % schema["delivery_size"] == 0,
                f"{label}: invalid delivery recipe",
            )
            require(
                not get("def_resolve") and not get("def_stock"),
                f"{label}: delivery conflicts with custom resolver",
            )
        row(get("def_row"), get("def_resolve") or get("def_stock") or delivery, label)
        icon = get("def_icon")
        require(icon != 0, f"{label}: missing icon")
        content.read(icon, 1)
        base = content.read(pointer + schema["def_base"], 1)[0]
        require(base < schema["main_rows"], f"{label}: invalid base row {base}")
        if get("def_levels"):
            curve(get("def_levels"), label)
        policy(get("def_policy"), label)
        timeline(get("def_timeline"), label)
        checked += 1

    def policy(pointer: int, label: str) -> None:
        if not pointer:
            return
        pose, cooldown, requirement = content.read(pointer, 3)
        require(1 <= pose <= schema["main_rows"], f"{label}: invalid animation pose")
        require(
            cooldown == 255 or cooldown < schema["main_rows"],
            f"{label}: invalid cooldown row",
        )
        require(
            requirement == 255 or requirement < schema["main_rows"],
            f"{label}: invalid requirement row",
        )
        content.read(pointer, schema["policy_size"])

    def timeline(pointer: int, label: str) -> None:
        if not pointer:
            return
        content.read(pointer, schema["timeline_size"])
        half = lambda offset: struct.unpack("<H", content.read(pointer + offset, 2))[0]
        total = half(schema["timeline_release"]) + half(schema["timeline_recovery"])
        count = content.read(pointer + schema["timeline_count"], 1)[0]
        require(total <= 3600 and count <= 16, f"{label}: invalid timeline bounds")
        events = content.pointer(pointer)
        require(not count or events != 0, f"{label}: missing timeline events")
        previous = 0
        for index in range(count):
            event = events + index * schema["event_size"]
            content.read(event, schema["event_size"])
            tick = struct.unpack("<H", content.read(event, 2))[0]
            effect = struct.unpack(
                "<H", content.read(event + schema["event_effect"], 2)
            )[0]
            anchor = content.read(event + schema["event_anchor"], 1)[0]
            require(previous <= tick <= total, f"{label}: invalid timeline event tick")
            require(
                effect < schema["effect_count"] and anchor <= 2,
                f"{label}: invalid timeline cue",
            )
            previous = tick

    registry = content.symbols["HeroKits"][0]
    fallback = content.symbols["SkillExpansionActiveDefs"][0]
    for hero in range(schema["heroes"]):
        kit = content.pointer(registry + hero * 4)
        identities: list[int] = []
        legacy_address, legacy_size = content.symbols["LegacyActiveIds"]
        legacy_ids = legacy_address + hero * (legacy_size // schema["heroes"])

        def identity(pointer: int, index: int) -> int:
            address = pointer or legacy_ids + index * 2
            value = struct.unpack("<H", content.read(address, 2))[0]
            require(
                value != 0, f"hero {hero} active {index}: missing permanent save ID"
            )
            identities.append(value)
            return value

        if kit:
            get = lambda name: content.pointer(kit + schema[name])
            active, icons, levels = (
                get("kit_active"),
                get("kit_icons"),
                get("kit_levels"),
            )
            require(
                active and icons and levels, f"hero {hero}: incomplete skill tables"
            )
            require(
                content.read(kit + schema["kit_art"], 1)[0] < 8,
                f"hero {hero}: invalid native tile art",
            )
            for index in range(schema["main_rows"]):
                label = f"hero {hero} active {index}"
                resolver = content.pointer(kit + schema["kit_resolvers"] + index * 4)
                row(active + index * schema["active_size"], resolver, label)
                icon = content.pointer(icons + index * 4)
                require(icon != 0, f"{label}: missing icon")
                content.read(icon, 1)
                curve(levels + index * schema["levels"], label)
                policy(content.pointer(kit + schema["kit_policies"] + index * 4), label)
                ids = get("kit_ids")
                identity(ids + index * 2 if ids else 0, index)
                checked += 1
            timelines = get("kit_timelines")
            if timelines:
                for index in range(schema["main_rows"] + schema["new_rows"]):
                    timeline(
                        content.pointer(timelines + index * 4),
                        f"hero {hero} timeline {index}",
                    )
            passives = list(content.read(get("kit_passives"), schema["main_passives"]))
            passives += list(
                content.read(kit + schema["kit_new_passives"], schema["new_passives"])
            )
            used = [value for value in passives if value != 255]
            require(
                all(value < schema["passive_end"] for value in used),
                f"hero {hero}: invalid passive ID",
            )
            require(
                len(used) == len(set(used)), f"hero {hero}: duplicate passive slots"
            )
        else:
            for index in range(schema["main_rows"]):
                identity(0, index)
        for index in range(schema["new_rows"]):
            pointer = (
                content.pointer(kit + schema["kit_new"] + index * 4)
                if kit
                else content.pointer(fallback + (hero * schema["new_rows"] + index) * 4)
            )
            if pointer:
                definition(pointer, f"hero {hero} active {index + 7}")
                id_address = pointer + schema["def_id"]
                value = struct.unpack("<H", content.read(id_address, 2))[0]
                identity(id_address if value else 0, index + 7)
        require(
            len(identities) == len(set(identities)),
            f"hero {hero}: duplicate permanent active IDs",
        )
    return checked


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    elf = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build/hack.elf"
    rom = Path(sys.argv[2]) if len(sys.argv) > 2 else root / "rotk.gba"
    try:
        count = validate(LinkedContent(elf.read_bytes(), rom.read_bytes()))
    except (ValueError, KeyError, struct.error) as error:
        raise SystemExit(f"Ability content validation failed: {error}") from error
    print(f"Ability content: {count} active definitions validated")


if __name__ == "__main__":
    main()
