"""Serve the local skill-effect editor without modifying source manifests."""

import argparse
import base64
from contextlib import redirect_stdout
from functools import partial
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from io import BytesIO, StringIO
import json
from pathlib import Path
import secrets
import shutil
import tempfile
import webbrowser
from zipfile import ZipFile, ZIP_DEFLATED

from PIL import Image
from skill_fx import ROOT, compile_sheet, generate as generate_fx, SHAPES, COLORS
from cast_timelines import generate as generate_timelines
from skill_deliveries import generate as generate_deliveries, LIMITS, SOUNDS

FILES = {
    "fx": "skill_fx.json",
    "timelines": "cast_timelines.json",
    "deliveries": "skill_deliveries.json",
}
STATIC = Path(__file__).with_suffix("")


def load_project(content: Path) -> dict:
    return {
        key: json.loads((content / name).read_text()) for key, name in FILES.items()
    }


def copy_assets(project: dict, content: Path, destination: Path) -> None:
    for asset in project["fx"]["assets"]:
        for field in ("sheet", "palette"):
            if field not in asset:
                continue
            relative = Path(asset[field])
            source = (content / relative).resolve()
            if (
                relative.is_absolute()
                or ".." in relative.parts
                or not source.is_relative_to(content.resolve())
                or source.suffix.lower() != ".png"
            ):
                raise ValueError("Asset paths must reference PNGs inside hacks/content")
            target = destination / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)


def validate_project(project: dict, content: Path) -> tuple[bytes, dict]:
    if not isinstance(project, dict) or set(project) != set(FILES):
        raise ValueError("Project must contain fx, timelines and deliveries")
    with tempfile.TemporaryDirectory(prefix="rotk-effect-editor-") as directory:
        scratch = Path(directory)
        source = scratch / "content"
        source.mkdir()
        for key, name in FILES.items():
            (source / name).write_text(
                json.dumps(project[key], indent=2) + "\n", newline="\n"
            )
        copy_assets(project, content, source)
        with redirect_stdout(StringIO()):
            generate_fx(source / FILES["fx"], scratch / "include", scratch / "preview")
            generate_timelines(
                source / FILES["timelines"],
                source / FILES["fx"],
                scratch / "include",
                scratch / "preview",
            )
            generate_deliveries(
                source / FILES["deliveries"],
                source / FILES["fx"],
                scratch / "include",
                scratch / "preview",
            )
        output = BytesIO()
        with ZipFile(output, "w", ZIP_DEFLATED) as archive:
            for path in source.rglob("*"):
                if path.is_file():
                    archive.write(
                        path, "hacks/content/" + path.relative_to(source).as_posix()
                    )
            archive.writestr("project.json", json.dumps(project, indent=2) + "\n")
            archive.writestr(
                "README.txt",
                "Validated with the ROM generators. Review changes before copying hacks/content into the repository. Run make check-fx and make hack afterward.\n",
            )
        return output.getvalue(), asset_catalog(project, source)


def asset_catalog(project: dict, content: Path) -> dict:
    assets = {}
    for spec in project["fx"]["assets"]:
        compiled = compile_sheet(spec, content)
        strip = Image.new("RGBA", (spec["width"] * compiled["frames"], spec["height"]))
        for index, frame in enumerate(compiled["previews"]):
            strip.paste(frame, (index * spec["width"], 0))
        image = BytesIO()
        strip.save(image, "PNG")
        assets[spec["key"]] = {
            key: compiled[key] for key in ("frames", "tileBytes", "romBytes", "colors")
        }
        assets[spec["key"]]["sheet"] = spec["sheet"]
        assets[spec["key"]]["image"] = (
            "data:image/png;base64," + base64.b64encode(image.getvalue()).decode()
        )
    return assets


def catalog(content: Path) -> dict:
    project = load_project(content)
    return {
        "project": project,
        "assets": asset_catalog(project, content),
        "shapes": list(SHAPES),
        "colors": list(COLORS),
        "limits": LIMITS,
        "sounds": list(SOUNDS),
    }


class Handler(BaseHTTPRequestHandler):
    def __init__(self, *args, content: Path, token: str, **kwargs):
        self.content = content
        self.token = token
        super().__init__(*args, **kwargs)

    def send(self, data: bytes, kind: str, status: int = 200) -> None:
        self.send_response(status)
        self.send_header("Content-Type", kind)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        self.wfile.write(data)

    def local_host(self) -> bool:
        allowed = {
            f"127.0.0.1:{self.server.server_port}",
            f"localhost:{self.server.server_port}",
        }
        if self.headers.get("Host") not in allowed:
            self.send(b"Local editor host required", "text/plain", 403)
            return False
        return True

    def do_GET(self) -> None:
        if not self.local_host():
            return
        if self.path == "/api/catalog":
            try:
                self.send(
                    json.dumps(catalog(self.content)).encode(), "application/json"
                )
            except (ValueError, KeyError, TypeError, OSError) as error:
                self.send(
                    json.dumps({"error": str(error)}).encode(), "application/json", 422
                )
            return
        paths = {
            "/": ("index.html", "text/html; charset=utf-8"),
            "/style.css": ("style.css", "text/css"),
            "/app.js": ("app.js", "text/javascript"),
            "/model.mjs": ("model.mjs", "text/javascript"),
        }
        if self.path not in paths:
            self.send(b"Not found", "text/plain", 404)
            return
        name, kind = paths[self.path]
        data = (
            (STATIC / name)
            .read_bytes()
            .replace(b"__EDITOR_TOKEN__", self.token.encode())
        )
        self.send(data, kind)

    def do_POST(self) -> None:
        if not self.local_host():
            return
        if self.path not in ("/api/validate", "/api/export"):
            self.send(b"Not found", "text/plain", 404)
            return
        if self.headers.get("X-Editor-Token") != self.token:
            self.send(b"Invalid editor token", "text/plain", 403)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 2_000_000:
                raise ValueError("Project must be smaller than 2 MB")
            project = json.loads(self.rfile.read(length))
            bundle, assets = validate_project(project, self.content)
            if self.path == "/api/export":
                self.send(bundle, "application/zip")
            else:
                self.send(
                    json.dumps({"valid": True, "assets": assets}).encode(),
                    "application/json",
                )
        except (ValueError, KeyError, TypeError, OSError, IndexError) as error:
            self.send(
                json.dumps({"error": str(error)}).encode(), "application/json", 422
            )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--open", action="store_true")
    args = parser.parse_args()
    server = ThreadingHTTPServer(
        ("127.0.0.1", args.port),
        partial(Handler, content=ROOT / "hacks/content", token=secrets.token_hex(24)),
    )
    url = f"http://127.0.0.1:{server.server_port}"
    print(f"Effect editor: {url} (Ctrl+C to stop)", flush=True)
    if args.open:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
