"""A Ghidra project for The Two Towers, named with ttt_names.py, and a batch decompiler for it.

    uv run python two-towers/tools/ttt_ghidra.py build        (about 10 minutes, once per name update)
    uv run python two-towers/tools/ttt_ghidra.py decomp ADDRESS|NAME ...

`build` writes build/ttt/ghidra_proj/ttt.gpr with the memory map of the ROTK project
(tools/rotkit/ghidra_build/gen_project.py), every named function created and labelled, a TttActor
struct on the effect functions' first parameter, then auto-analysis. Rerun it after adding names to
two-towers/ttt_labels.txt. It is separate from ghidra_proj/, so no Two Towers name reaches the ROTK
project. `decomp` prints the decompiled C, also cached in build/ttt/decomp/. Needs JAVA_HOME and
GHIDRA_INSTALL_DIR (docs/setup.md), the matcher's build/ttt/symbols.json and the Two Towers ROM.

For the MCP server, run `%LOCALAPPDATA%\\rotk-tools\\start-ghidra-mcp-ttt.cmd` (port 8081).
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

import ttt_names
from roms import ROM_BASE, TTT_ROM

import rotkit.pyghidra_env  # noqa: F401  (headless AWT)

PROJECT_DIR = ttt_names.ROOT / "build/ttt/ghidra_proj"
DECOMP_DIR = ttt_names.ROOT / "build/ttt/decomp"
PROJECT = "ttt"

# Two Towers actor fields known so far. The header up to 0x20 is ROTK's.
ACTOR_FIELDS = [
    (0x00, "next", "void *"),
    (0x04, "previous", "void *"),
    (0x08, "ownerChar", "void *"),
    (0x0C, "parentActor", "TttActor *"),
    (0x10, "renderFollowTarget", "TttActor *"),
    (0x14, "tickCallback", "void *"),
    (0x18, "triggerCallback0", "void *"),
    (0x1C, "triggerCallback1", "void *"),
    (0x58, "actionStateTimer", "int"),
    (0x67, "actionState", "byte"),
    (0x75, "radius", "byte"),
    (0x90, "damage", "ushort"),
    (0x9A, "playerIndex", "byte"),
    (0xFA, "facing", "byte"),
]
ACTOR_SIZE = 0x100
# bytes cleared after a function that analysis did not find, before it is decoded as Thumb
PROBE_CLEAR_BYTES = 0x200
# Functions whose first parameter is an actor (effects, their callbacks, triggers) or a player
# index (casts).
ACTOR_FUNCTION = re.compile(r"^(fx_|trigger_|player_tick$)")
CAST_FUNCTION = re.compile(r"^ttt_.*_cast$")


def build() -> None:
    import pyghidra
    pyghidra.start()

    from ghidra.app.cmd.disassemble import ArmDisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.app.plugin.core.analysis import AutoAnalysisManager
    from ghidra.base.project import GhidraProject
    from ghidra.program.database import ProgramDB
    from ghidra.program.flatapi import FlatProgramAPI
    from ghidra.program.model.data import (ByteDataType, CategoryPath, IntegerDataType,
                                           PointerDataType, StructureDataType,
                                           UnsignedShortDataType, VoidDataType)
    from ghidra.program.model.lang import LanguageID
    from ghidra.program.model.listing import ParameterImpl, Program
    from ghidra.program.model.listing.Function import FunctionUpdateType
    from ghidra.program.model.symbol import SourceType
    from ghidra.program.util import DefaultLanguageService
    from ghidra.util.task import ConsoleTaskMonitor
    from java.io import File, FileInputStream
    from java.lang import Object
    from java.math import BigInteger
    from rotkit.ghidra_build.gen_project import BLOCKS, IO_BASE, IO_REGISTERS

    monitor = ConsoleTaskMonitor()
    rom = ttt_names.rom_bytes()
    functions, globals_ = ttt_names.matched()
    if not functions:
        raise SystemExit("no build/ttt/symbols.json: run two-towers/tools/ttt_match.py first")
    code = dict(functions)
    code.update({s.cast: s.symbol for s in ttt_names.skills(rom)})
    data = dict(globals_)
    for address, (name, comment) in ttt_names.labels().items():
        (code if name[0].islower() else data)[address] = name

    shutil.rmtree(PROJECT_DIR, ignore_errors=True)
    PROJECT_DIR.mkdir(parents=True)
    lang = DefaultLanguageService.getLanguageService().getLanguage(LanguageID("ARM:LE:32:v4t"))
    consumer = Object()
    program = ProgramDB(PROJECT, lang, lang.getDefaultCompilerSpec(), consumer)
    space = program.getAddressFactory().getDefaultAddressSpace()
    addr = space.getAddress
    mem, st, fm = program.getMemory(), program.getSymbolTable(), program.getFunctionManager()
    flat, dtm = FlatProgramAPI(program), program.getDataTypeManager()

    tx = program.startTransaction("ttt")
    try:
        fb = mem.createFileBytes("ttt.gba", 0, len(rom), FileInputStream(File(str(TTT_ROM))), monitor)
        for blk in BLOCKS:
            if blk.file_offset is not None:
                size = blk.size if blk.size is not None else len(rom) - blk.file_offset
                block = mem.createInitializedBlock(blk.name, addr(blk.start), fb, blk.file_offset, size, False)
            else:
                block = mem.createUninitializedBlock(blk.name, addr(blk.start), blk.size, False)
            block.setRead(blk.readable)
            block.setWrite(blk.writable)
            block.setExecute(blk.executable)
        st.addExternalEntryPoint(addr(ROM_BASE))
        for reg in IO_REGISTERS:
            st.createLabel(addr(IO_BASE + reg.offset), reg.name, SourceType.USER_DEFINED)
        # see gen_project.py: the ARM Symbol analyzer shifts odd-addressed labels
        program.getOptions(Program.ANALYSIS_PROPERTIES).setBoolean("ARM Symbol", False)

        category = CategoryPath("/ttt")
        actor = StructureDataType(category, "TttActor", ACTOR_SIZE, dtm)
        actor = dtm.addDataType(actor, None)
        actor_ptr = PointerDataType(actor)
        types = {"void *": PointerDataType(VoidDataType.dataType), "TttActor *": actor_ptr,
                 "int": IntegerDataType.dataType, "byte": ByteDataType.dataType,
                 "ushort": UnsignedShortDataType.dataType}
        for offset, name, type_name in ACTOR_FIELDS:
            dt = types[type_name]
            actor.replaceAtOffset(offset, dt, dt.getLength(), name, None)

        tmode = program.getRegister("TMode")
        ctx = program.getProgramContext()
        for address in code:
            ctx.setValue(tmode, addr(address), addr(address), BigInteger.ONE)
        made = 0
        for address, name in sorted(code.items()):
            a = addr(address)
            ArmDisassembleCommand(a, None, True).applyTo(program, monitor)
            CreateFunctionCmd(name, a, None, SourceType.USER_DEFINED).applyTo(program, monitor)
            func = fm.getFunctionAt(a)
            if func is None:
                print(f"  no function at {address:#x} {name}")
                continue
            func.setName(name, SourceType.USER_DEFINED)
            param = (ParameterImpl("actor", actor_ptr, program) if ACTOR_FUNCTION.match(name) else
                     ParameterImpl("playerIndex", ByteDataType.dataType, program)
                     if CAST_FUNCTION.match(name) else None)
            if param is not None:
                func.replaceParameters(FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, True,
                                       SourceType.USER_DEFINED, [param])
            made += 1
        for address, name in data.items():
            if mem.contains(addr(address)):
                st.createLabel(addr(address), name, SourceType.USER_DEFINED)
        for address, (name, comment) in ttt_names.labels().items():
            if comment:
                flat.setPlateComment(addr(address), comment)
        print(f"{made}/{len(code)} functions, {len(data)} data labels")
    finally:
        program.endTransaction(tx, True)

    print("auto-analysis...")
    tx = program.startTransaction("analyze")
    try:
        mgr = AutoAnalysisManager.getAnalysisManager(program)
        mgr.initializeOptions()
        mgr.reAnalyzeAll(None)
        mgr.startAnalysis(monitor)
    finally:
        program.endTransaction(tx, True)
    print(f"{fm.getFunctionCount()} functions after analysis")

    project = GhidraProject.createProject(str(PROJECT_DIR), PROJECT, False)
    project.saveAs(program, "/", PROJECT, True)
    try:
        program.release(consumer)
        project.close()
    except Exception:
        pass
    for lock in PROJECT_DIR.glob("*.lock*"):
        try:
            lock.unlink()
        except OSError:
            pass  # still held until this process exits
    shutil.rmtree(DECOMP_DIR, ignore_errors=True)
    print(f"saved {PROJECT_DIR / (PROJECT + '.gpr')}")


class Decompiler:
    """Opens the project read-only once and decompiles many functions."""

    def __init__(self):
        import pyghidra
        pyghidra.start()
        from ghidra.app.decompiler import DecompInterface
        from ghidra.base.project import GhidraProject
        if not (PROJECT_DIR / f"{PROJECT}.gpr").exists():
            raise SystemExit("no Two Towers Ghidra project: run ttt_ghidra.py build")
        self.project = GhidraProject.openProject(str(PROJECT_DIR), PROJECT, True)
        self.program = self.project.openProgram("/", PROJECT, True)
        self.space = self.program.getAddressFactory().getDefaultAddressSpace()
        self.decomp = DecompInterface()
        self.decomp.openProgram(self.program)

    def resolve(self, what: str) -> int:
        if re.fullmatch(r"(0x)?[0-9a-fA-F]{7,8}", what):
            return int(what, 16) & ~1
        for symbol in self.program.getSymbolTable().getSymbols(what):
            return symbol.getAddress().getOffset()
        raise SystemExit(f"no symbol {what}")

    def function(self, address: int):
        from ghidra.app.cmd.disassemble import ArmDisassembleCommand
        from ghidra.app.cmd.function import CreateFunctionCmd
        from ghidra.util.task import ConsoleTaskMonitor
        fm = self.program.getFunctionManager()
        a = self.space.getAddress(address)
        func = fm.getFunctionAt(a)
        if func is None:
            # read-only project: create in a throwaway transaction, never saved. Analysis may have
            # decoded the bytes another way (as ARM, or inside other code); clear them first.
            from java.math import BigInteger
            tx = self.program.startTransaction("probe")
            ctx = self.program.getProgramContext()
            listing = self.program.getListing()
            listing.clearCodeUnits(a.subtract(2), a.add(PROBE_CLEAR_BYTES), False)
            ctx.setValue(self.program.getRegister("TMode"), a, a.add(PROBE_CLEAR_BYTES), BigInteger.ONE)
            ArmDisassembleCommand(a, None, True).applyTo(self.program, ConsoleTaskMonitor())
            CreateFunctionCmd(a).applyTo(self.program, ConsoleTaskMonitor())
            self.program.endTransaction(tx, True)
            func = fm.getFunctionAt(a)
        return func

    def c(self, address: int) -> str:
        cached = DECOMP_DIR / f"{address:08x}.c"
        if cached.exists():
            return cached.read_text()
        func = self.function(address)
        if func is None:
            return f"// no function at {address:#x}\n"
        result = self.decomp.decompileFunction(func, 60, None)
        text = result.getDecompiledFunction().getC() if result.decompileCompleted() else \
            f"// decompile failed at {address:#x}: {result.getErrorMessage()}\n"
        DECOMP_DIR.mkdir(parents=True, exist_ok=True)
        cached.write_text(text)
        return text

    def name(self, address: int) -> str:
        symbol = self.program.getSymbolTable().getPrimarySymbol(self.space.getAddress(address))
        return symbol.getName() if symbol else f"{address:#x}"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("build")
    p = sub.add_parser("decomp")
    p.add_argument("functions", nargs="+")
    args = parser.parse_args()
    if args.command == "build":
        build()
    else:
        d = Decompiler()
        for what in args.functions:
            print(d.c(d.resolve(what)))


if __name__ == "__main__":
    main()
