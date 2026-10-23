"""Import Saruman's selected skill artwork and rebuild his native icons and preview."""

import argparse
import json
from pathlib import Path
from types import SimpleNamespace

from PIL import Image, ImageDraw

import skill_icon_art

ROOT = Path(__file__).resolve().parents[2]
CONTENT = ROOT / "hacks/content/icons/saruman"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--import-sheets", action="store_true")
    args = parser.parse_args()
    manifest = json.loads((CONTENT / "manifest.json").read_text())
    selected = []
    for sheet in manifest["sheets"]:
        names = ["-"] * 24
        for icon in sheet["icons"]:
            names[icon["slot"] - 1] = icon["key"]
            selected.append((sheet["source"], icon))
        if args.import_sheets:
            skill_icon_art.cmd_slice(
                SimpleNamespace(
                    mockup=str(CONTENT / sheet["source"]),
                    out_dir=str(CONTENT / "png"),
                    grid=sheet["grid"],
                    names=",".join(names),
                    backdrop=skill_icon_art.BACKDROP,
                )
            )
    skill_icon_art.cmd_build(
        SimpleNamespace(
            png_dir=str(CONTENT / "png"),
            out=str(ROOT / "hacks/patches/ringwraith/saruman_icons.c"),
            prefix="SarumanIcon",
        )
    )
    preview = Image.new("RGB", (1140, 400), "#242424")
    draw = ImageDraw.Draw(preview)
    for i, (_, icon) in enumerate(selected):
        x, y = (i % 6) * 190, (i // 6) * 80
        with Image.open(CONTENT / "png" / (icon["key"] + ".png")) as image:
            scaled = image.convert("RGBA").resize((64, 48), Image.Resampling.NEAREST)
            preview.paste(scaled, (x + 60, y), scaled)
        draw.text((x + 4, y + 52), icon["key"], fill="white")
    output = ROOT / "build/saruman_icons_preview.png"
    output.parent.mkdir(exist_ok=True)
    preview.save(output)
    print(f"Preview: {output}")


if __name__ == "__main__":
    main()
