# items.gdb: seed all unique items into the hack's Ranger Hollow pages 70-75.
#
# Build matching ROM/symbols with `make hack debug`, then:
#   gdb -x build/debug/rotk_hack.gdb -x gdb-scripts/items.gdb
#   (gdb) hollow_uniques
#   (gdb) continue
#
# Back up your save first. Run in single-player with the Hollow closed.
# `hollow_uniques force` replaces occupied cells in those six pages.
# Items are uncarved, without affixes. The final unused cell is cleared.
# Writes go directly to SRAM, take effect without an in-game save, and are read back.
# Reopen the Hollow to load them. No inferior calls or equipment/stat edits.

python
import gdb
import re
from pathlib import Path


def _hollow_value(expression: str) -> int:
    return int(gdb.parse_and_eval(expression))


def _hollow_layout() -> tuple[int, int, int, bytes]:
    prog = gdb.current_progspace()
    try:
        objects = prog.objfiles()
    except TypeError:
        objects = prog.objfiles
    root = next((Path(obj.filename).resolve().parents[2] for obj in objects
                 if Path(obj.filename).name == "debug.elf"), None)
    if root is None:
        raise gdb.GdbError("Load build/debug/rotk_hack.gdb first.")
    if gdb.lookup_global_symbol("rangerHollowPages_onShopEnter") is None:
        raise gdb.GdbError("Ranger Hollow patch symbols are not loaded. "
                           "If running the matching hack ROM, run "
                           "`add-symbol-file build/hack.elf`, then retry. "
                           "The build must include ranger_hollow_pages.")

    layout = (root / "hacks/patches/mod_save/save_layout.h").read_text()

    def number(name: str) -> int:
        match = re.search(r"^#define\s+" + name + r"\s+(0x[0-9a-fA-F]+|[0-9]+)[uU]?\s*$",
                          layout, re.M)
        if match is None:
            raise gdb.GdbError(f"Cannot read {name}. Check save_layout.h.")
        return int(match[1], 0)

    magic = re.search(r'^#define\s+SAVE_HEADER_MAGIC\s+"([^"]+)"', layout, re.M)
    if magic is None:
        raise gdb.GdbError("Cannot read SAVE_HEADER_MAGIC.")
    header = magic[1].encode("ascii") + bytes([number("SAVE_HEADER_VERSION")])
    base = _hollow_value("sizeof(SaveGlobals)") + number("SAVE_SLOT_PAGE_COUNT") * \
        _hollow_value("sizeof(g_SaveSlots)")
    page_size = _hollow_value("sizeof(g_PlayerGlobals.players[0].rangerHollowItems)")
    return base, page_size, number("HOLLOW_PAGE_COUNT"), header


def _hollow_settle() -> None:
    saved = [(bp, bp.enabled) for bp in gdb.breakpoints() or []]
    try:
        for bp, enabled in saved:
            if bp.number >= 0:
                bp.enabled = False
        gdb.execute("settle")
        expected = (_hollow_value("(unsigned long)rotk_main") & ~1) + 0xae
        if _hollow_value("$pc") != expected:
            raise gdb.GdbError("Did not reach the main-loop tick. Nothing written.")
    finally:
        for bp, enabled in saved:
            if bp.is_valid():
                bp.enabled = enabled


def _hollow_write(inferior: gdb.Inferior, address: int, data: bytes) -> None:
    # SRAM has an 8-bit bus. Keep debugger writes byte-sized too.
    for offset, value in enumerate(data):
        inferior.write_memory(address + offset, bytes([value]))


def _hollow_uniques(force: bool) -> None:
    first_page = 70  # Displayed page number, not the zero-based SRAM index.
    sram = 0x0e000000
    base, page_size, page_count, magic = _hollow_layout()
    item_size = _hollow_value("sizeof(Item)")
    count = _hollow_value("sizeof(UniqueItems) / sizeof(UniqueItems[0])")
    if item_size != 4 or page_size < item_size or page_size % item_size or not 1 <= count <= 256:
        raise gdb.GdbError("Unexpected item layout. Nothing written.")
    slots = page_size // item_size
    pages = (count + slots - 1) // slots
    last_page = first_page + pages - 1
    if last_page > page_count or base + page_count * page_size > 0x8000:
        raise gdb.GdbError("Collection does not fit the Hollow layout. Nothing written.")

    _hollow_settle()
    if _hollow_value("g_GameFlags.d.coopActive"):
        raise gdb.GdbError("Leave co-op before seeding the Hollow.")
    if _hollow_value("g_SceneCurrent.id == SCENE_ID_SHOP && "
                     "g_ShopType == SHOP_TYPE_RANGER_HOLLOW"):
        raise gdb.GdbError("Close the Ranger Hollow first, then rerun hollow_uniques.")
    if _hollow_value("g_SceneNext.id & 0x80"):
        raise gdb.GdbError("A scene change is pending. Let it finish first.")

    inferior = gdb.selected_inferior()
    header = bytes(inferior.read_memory(sram, _hollow_value("sizeof(SaveGlobals)")))
    if header[:len(magic)] != magic or sum(
            int.from_bytes(header[i:i + 2], "little") for i in range(0, len(header), 2)) & 0xffff:
        raise gdb.GdbError("SRAM header is not a valid current mod save. Nothing written.")

    item_type = _hollow_value("ITEM_TYPE_UNIQUE")
    rune_none = _hollow_value("RUNE_NONE")
    payload = b"".join((0xffff0000 | index << 8 | rune_none << 4 | item_type).to_bytes(4, "little")
                       for index in range(count))
    payload = payload.ljust(pages * page_size, b"\xff")
    address = sram + base + (first_page - 1) * page_size
    before = bytes(inferior.read_memory(address, len(payload)))
    if not force:
        for offset in range(0, len(payload), item_size):
            old = before[offset:offset + item_size]
            new = payload[offset:offset + item_size]
            if old != b"\xff" * item_size and old != new:
                raise gdb.GdbError(f"Pages {first_page}-{last_page} contain other items. "
                                   "Back up your save, then use hollow_uniques force to replace them.")
    try:
        _hollow_write(inferior, address, payload)
        if bytes(inferior.read_memory(address, len(payload))) != payload:
            raise gdb.GdbError("SRAM readback did not match.")
    except Exception:
        try:
            _hollow_write(inferior, address, before)
            if bytes(inferior.read_memory(address, len(before))) != before:
                raise gdb.GdbError("Rollback readback did not match.")
        except Exception:
            gdb.write("Rollback failed. Restore your save backup before continuing.\n")
        raise
    gdb.write(f"Verified {count} uniques in Hollow pages {first_page}-{last_page}. "
              "Continue, then reopen the Hollow.\n")
end

define hollow_uniques
    if $argc == 0
        python _hollow_uniques(False)
    else
        if $argc == 1
            if $_streq("$arg0", "force")
                python _hollow_uniques(True)
            else
                printf "usage: hollow_uniques [force]\n"
            end
        else
            printf "usage: hollow_uniques [force]\n"
        end
    end
end

document hollow_uniques
Seed all uniques into displayed Ranger Hollow pages 70-75 (hack build only).
Close the Hollow first. Runs settle and leaves the game paused afterward.
Existing matching items are allowed. Use `hollow_uniques force` to replace other items.
Back up your save first: this changes SRAM immediately, including the last unused cell.
end

printf "items.gdb loaded: hollow_uniques [force] (pages 70-75)\n"
