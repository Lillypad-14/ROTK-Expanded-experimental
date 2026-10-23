# Mission loading follow-up

Promoted mission_script_loadMissionBlock (0x0800c044, 168 bytes) and mission_script_copySpawnRecords (0x0800c6e0, 72 bytes). Header structures now describe the mission header, eight-byte spawn records, variant chunk index table and eight-byte chunk references. Existing Python missionscript parser confirms eleven variant slots and the record layouts. The state pointer at 0x0300324c is now mapped as g_MissionScriptState, MissionBlockHeader *. It is a pointer to the arena, not the arena itself.

The loader calls copySpawnRecords with two arguments (block and decompressed variant chunk), although the copier does not use the second. The previous one-argument prototype caused Ghidra to hide the second argument; current header corrects it. decompress_chunk now has a hand-written declaration returning u32 in include/decompress.h; assembly confirms the decoded byte count in r0.

Loader requires source pinned to r2 and MATCH_BARRIER after calculating the variant-table address. Exhaustive eight-combination audit found those two together match with or without an offset pin; all smaller combinations differ. Offset pin removed. Copier matches plain typed struct assignment, with a local headerSize to preserve original load order and avoid a second aliased header read.

Next: mission_script_loadStreams (Ghidra evidence in mission-loadStreams-ghidra.txt and mission-loadStreams-asm.txt) uses two unmapped alignment helpers. Ghidra disassemble_bytes at 0x0800c7d8 length72 showed two distinct functions, not one: 0x0800c7d8..0x0800c7f8 rounds a u16 halfword count up to even, doubles it with u16 narrowing, returns u16; 0x0800c7f8..0x0800c820 rounds a u16 byte count to the next multiple of four with wraparound, spelling the remainder cases separately. Neither is mapped or decompiled yet. Recover their callers via Ghidra before choosing final module names. The previous copyStream candidate list length140 included both helpers; actual copyStream is only68 bytes.

Do not confuse record tables with decompressed chunk headers: a variant chunk begins with u16 waypoint section offset, u16 stream section offset and u16 flags. loadStreams uses the stream-count byte, prefixes its table with a u16 count, optionally prepends shared stream0 when g_MissionFlags bit0 is set, and builds aligned u16 offsets. Raw pseudo is missing indirect-call/epilogue details; use assembly.

Checks: mission-load-check.log and mission-load-ghidra-tests.log.

Validation complete: make check passed, 443/443 regions, original SHA-1, 297 tests passed with three default skips. Configured Ghidra files passed all18 tests.
