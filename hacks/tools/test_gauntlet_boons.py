"""Exercise Gauntlet ownership, offers, statuses, cast triggers and cleanup without a ROM."""

from pathlib import Path
import shutil
import subprocess
import tempfile


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("clang") or shutil.which("cc")
    if compiler is None:
        raise SystemExit("Install clang or cc to run the boon tests.")
    with tempfile.TemporaryDirectory(prefix="rotk-boon-tests-") as directory:
        executable = Path(directory) / "gauntlet-boons.exe"
        subprocess.run(
            [
                compiler,
                "-std=gnu11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-Wno-unused-parameter",
                *(f"-I{root / path}" for path in ("include", "carved/include", "build/include", "build/hack_include", "hacks")),
                str(root / "hacks/tests/gauntlet_boons.c"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
