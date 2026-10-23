# Mission table loading

Promoted mission_script_getAlignedHalfwordBytes (0x0800c7d8), mission_script_alignByteSize (0x0800c7f8), mission_script_loadStreams (0x0800c3ac, 220 bytes) and mission_script_loadWaypointPaths (0x0800c2cc, 224 bytes). Alignment helpers share src/c/mission_script_alignment.c: its compiled region is 70 bytes, including inter-function padding but excluding the final two original padding bytes. Total new compiled bytes: 514. None of these functions needs matching macros.

Incoming Ghidra calls to halfword helper: 0x0800c1bc, 0x0800c27a, 0x0800c310, 0x0800c3f8. Byte-align helper: 0x0800c26c. All are mission loaders. Both return u16 and preserve wrapping.

Added typed chunk and stream-section headers and MISSION_FLAG_SHARED_CHUNK. g_MissionStreams is now MissionOffsetTable *. Existing runStream and loadMissionBlock consumers were adjusted with explicit byte-view casts and rematched exactly. Corrected the old comment that treated 0x0300324c as an inline arena; it holds the arena pointer.

Stream-loader first candidate held a redundant typed header pointer across calls, consuming sl. Accessing the chunk header at each use removes that duplicate. Reusing one u16 offset across initial source-section selection, table-size calculation, shared-section selection and record offsets matches all 220 bytes naturally. Waypoint loader matches plain C once each output offset is computed into a local before addressing the output offset slot.

Next: mission_script_copyWaypointPath at 0x0800c750 is mapped and declared, not promoted. Ghidra assembly confirms a u8 input count widened to a u32 output count, followed by count eight-byte waypoint records and a returned destination cursor. Initial struct assignment emits ldr/str pairs; libc memcpy emits the original ldmia/stmia.

Natural typed candidate mission_script_copyWaypointPath_candidate.c differs by four instructions (two commutative address adds and two final size-register operations). path-aligned.c reduces this to two final register differences, but uses raw address arithmetic and typed-pointer alignment casts; prefer improving the natural candidate before promotion. path-final.c and path-two.c are unsuccessful pin probes. Byte-pointer copies lose alignment and expand into byte stores. The explicit typed casts in path-aligned.c recover the known alignment. No probe has been promoted.

Remaining mission pipeline: mission_script_loadGroups, mission_script_copyGroup, then the waypoint copier. Avoid counting prior estimated gaps as function lengths. All target discovery and assembly came from GhidrAssist.

Validation logs: mission-tables-check.log and mission-tables-ghidra-tests.log.
