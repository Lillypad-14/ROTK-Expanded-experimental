"""Exercise the native font rasterizer with Gauntlet's overlapping tile rows."""

from pathlib import Path
import shutil
import re
import subprocess
import tempfile


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    if compiler is None:
        raise SystemExit("Install clang or cc to run the font regression.")
    with tempfile.TemporaryDirectory(prefix="rotk-font-tests-") as directory:
        executable = Path(directory) / "font-tests.exe"
        subprocess.run(
            [
                compiler,
                "-std=c11",
                "-Wno-int-to-pointer-cast",
                "-Wno-pointer-to-int-cast",
                *(
                    f"-I{root / path}"
                    for path in ("include", "carved/include", "build/include", "hacks")
                ),
                str(root / "hacks/tests/gauntlet_text.c"),
                str(root / "src/c/font.c"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        table = re.search(
            r"^0x([0-9a-fA-F]+)\s+FontStyleInfos\s",
            (root / "config/data.cfg").read_text(),
            re.MULTILINE,
        )
        if table is None:
            raise SystemExit("FontStyleInfos is missing from config/data.cfg")
        subprocess.run(
            [
                str(executable),
                str(root / "rotk.gba"),
                str(int(table[1], 16) - 0x08000000),
            ],
            check=True,
        )


if __name__ == "__main__":
    main()
