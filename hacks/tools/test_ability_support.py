"""Run shared ability helper regressions with a host C compiler after make hack."""

from pathlib import Path
import shutil
import subprocess
import tempfile


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    if compiler is None:
        raise SystemExit("Install clang or cc to run the host helper tests.")
    with tempfile.TemporaryDirectory(prefix="rotk-ability-tests-") as directory:
        executable = Path(directory) / "ability-tests.exe"
        subprocess.run(
            [
                compiler,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                *(
                    f"-I{root / path}"
                    for path in ("include", "carved/include", "build/include", "hacks")
                ),
                str(root / "hacks/tests/ability_support.c"),
                str(root / "hacks/lib/movement.c"),
                str(root / "hacks/lib/combat_source.c"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run([str(executable)], check=True)
        engine = Path(directory) / "engine-tests.exe"
        subprocess.run(
            [
                compiler,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                *(
                    f"-I{root / path}"
                    for path in ("include", "carved/include", "build/include", "hacks")
                ),
                *(
                    str(root / path)
                    for path in (
                        "hacks/tests/engine_support.c",
                        "hacks/lib/actor_ref.c",
                        "hacks/lib/profile.c",
                        "hacks/lib/targeting.c",
                        "hacks/lib/timed_effect.c",
                        "hacks/patches/skill_expansion/skill_status.c",
                        "hacks/lib/summon_registry.c",
                        "hacks/lib/combat_source.c",
                        "hacks/patches/skill_expansion/skill_expansion_save_format.c",
                    )
                ),
                "-o",
                str(engine),
            ],
            check=True,
        )
        subprocess.run([str(engine)], check=True)
        save = Path(directory) / "save-tests.exe"
        subprocess.run(
            [
                compiler,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-DHACK_mod_save=1",
                *(
                    f"-I{root / path}"
                    for path in (
                        "include",
                        "carved/include",
                        "build/include",
                        "build/hack_include",
                        "hacks",
                    )
                ),
                str(root / "hacks/tests/skill_save.c"),
                str(
                    root / "hacks/patches/skill_expansion/skill_expansion_save_format.c"
                ),
                "-o",
                str(save),
            ],
            check=True,
        )
        subprocess.run([str(save)], check=True)
        helpers = Path(directory) / "heap_helpers.c"
        native = (root / "src/c/memory.c").read_text()
        helpers.write_text(
            '#include "memory.h"\n#include "variables.h"\n'
            "#define HEAP_BLOCK_ALLOCATED ((HeapBlock *)-1)\n"
            + native[
                native.index(
                    "static u32 usableSize(HeapBlock *block, u32 heapIndex)\n{"
                ) :
            ]
            .replace("static u32", "u32")
            .replace("static void", "void")
        )
        memory = Path(directory) / "memory-tests.exe"
        subprocess.run(
            [
                compiler,
                "-std=gnu11",
                "-Wall",
                "-Wextra",
                "-Werror",
                *(
                    f"-I{root / path}"
                    for path in ("include", "carved/include", "build/include", "hacks")
                ),
                str(helpers),
                str(root / "hacks/tests/memory_support.c"),
                str(root / "hacks/patches/mod_ram/memory_diagnostics.c"),
                str(root / "hacks/patches/mod_ram/graphics_scratch.c"),
                "-o",
                str(memory),
            ],
            check=True,
        )
        subprocess.run([str(memory)], check=True)
        saruman = Path(directory) / "saruman-tests.exe"
        subprocess.run(
            [
                compiler,
                "-std=gnu11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-DHACK_skill_expansion=1",
                *(
                    f"-I{root / path}"
                    for path in (
                        "include",
                        "carved/include",
                        "build/include",
                        "build/hack_include",
                        "hacks",
                    )
                ),
                str(root / "hacks/tests/saruman.c"),
                "-o",
                str(saruman),
            ],
            check=True,
        )
        subprocess.run([str(saruman)], check=True)


if __name__ == "__main__":
    main()
