# Environment Setup

## Required

- `git`
- A normal (host system) C toolchain (`cc`, `make`) to build `agbcc`
- `arm-none-eabi-gcc` and `arm-none-eabi-binutils`
- `agbcc`: buildable using `tools/setup_agbcc.sh`
- `uv`: Python package management for `rotkit`
- `libclang`: Used by `rotkit`. Typically you get it when you install `clang`.
- `clang-format`: automatic formatting using `make format`
- `armips` (for assembling the ROM hack): [https://github.com/Kingcom/armips](https://github.com/Kingcom/armips)
- `flips` (for building ROM hack patch files): [https://git.disroot.org/Sir_Walrus/Flips](https://git.disroot.org/Sir_Walrus/Flips)
- The original game ROM as `rotk.gba`

## Optional

- Ghidra (if you want to use the `make ghidra` functionality)
- [objdiff](https://github.com/encounter/objdiff): interactive matched-function diff, paired with
  `make objdiff` (see [workflow.md](workflow.md)).
- `clangd`: editor indexing. `make clangd` regenerates `compile_commands.json` and
  `hacks/compile_flags.txt` (whose `-DHACK_*` lines track `hacks/patches.cfg`).

## Checks

- `make check-tools` checks for missing tools, reports the Ghidra version, and verifies the ROM checksum.
- `make verify` rebuilds the whole ROM from source and is the real test that the setup works.

## Git Hooks

`make install-hooks` installs git hooks to format/check before committing.

## Windows Without WSL

The mod build (`make hack`) runs natively from Git Bash. The full decompilation checks (`make verify`) still
expect Linux or WSL.

1. Install [Git for Windows](https://gitforwindows.org/) and work from Git Bash.
2. Install `make` and `uv`, for example `winget install ezwinports.make astral-sh.uv`.
3. Install [LLVM](https://github.com/llvm/llvm-project/releases) with the Windows installer to the default
   `C:\Program Files\LLVM`. `rotkit` loads `libclang.dll` from there when the pip package cannot.
4. Unzip the [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
   (`arm-none-eabi`) and [armips](https://github.com/Kingcom/armips/releases) anywhere, and put their `bin`
   folders on `PATH`.
5. Clone, copy your ROM in as `rotk.gba`, then:

```sh
uv sync
make hack          # writes build/rotk_hack.gba
```

The repository stores text with LF line endings through `.gitattributes`, so `core.autocrlf` does not matter.

To check the mod after a change, run the hero sweep from [test-arena.md](test-arena.md) with mGBA.
