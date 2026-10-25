"""Exercise the actual shop C implementation without a ROM or emulator."""

from pathlib import Path
import shutil
import subprocess


def test_shop(tmp_path: Path) -> None:
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    assert compiler is not None, "Install clang or cc for the shop regression."
    executable = tmp_path / "shop-tests.exe"
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
            str(root / "hacks/tests/gauntlet_shop.c"),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
