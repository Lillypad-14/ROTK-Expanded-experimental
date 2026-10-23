"""Exercise editor export validation, source isolation and browser playback math."""

from functools import partial
from http.client import HTTPConnection
from http.server import ThreadingHTTPServer
from io import BytesIO
import json
from pathlib import Path
import shutil
import subprocess
from threading import Thread
from zipfile import ZipFile

from PIL import Image
import pytest
from effect_editor import FILES, Handler, ROOT, catalog, validate_project


@pytest.fixture
def content(tmp_path: Path) -> tuple[Path, dict]:
    image = Image.new("RGBA", (16, 8), (0, 0, 0, 0))
    image.putpixel((0, 0), (255, 0, 0, 255))
    image.putpixel((8, 0), (0, 0, 0, 255))
    image.save(tmp_path / "sheet.png")
    delivery = json.loads((ROOT / "hacks/content/skill_deliveries.json").read_text())[
        "deliveries"
    ][0]
    delivery["travelEffect"] = delivery["impactEffect"] = "test"
    project = {
        "fx": {
            "version": 1,
            "assets": [
                {
                    "key": "sprite",
                    "id": 0,
                    "sheet": "sheet.png",
                    "width": 8,
                    "height": 8,
                    "frameTicks": 3,
                }
            ],
            "effects": [
                {
                    "key": "test",
                    "id": 0,
                    "name": "Test",
                    "layers": [{"asset": "sprite"}],
                }
            ],
        },
        "timelines": {
            "version": 1,
            "timelines": [
                {
                    "key": "quick",
                    "id": 0,
                    "name": "Quick",
                    "releaseTick": 6,
                    "recoveryTicks": 9,
                    "events": [{"tick": 6, "effect": "test", "anchor": "owner"}],
                }
            ],
        },
        "deliveries": {"version": 1, "deliveries": [delivery]},
    }
    for key, name in FILES.items():
        (tmp_path / name).write_text(json.dumps(project[key]))
    return tmp_path, project


def test_export_and_source_isolation(content: tuple[Path, dict]) -> None:
    directory, project = content
    before = {path.name: path.read_bytes() for path in directory.iterdir()}
    project["fx"]["assets"][0]["anchor"] = [2, 3]
    bundle, assets = validate_project(project, directory)
    with ZipFile(BytesIO(bundle)) as archive:
        assert json.loads(archive.read("project.json")) == project
        assert archive.read("hacks/content/sheet.png") == before["sheet.png"]
        for key, name in FILES.items():
            assert json.loads(archive.read("hacks/content/" + name)) == project[key]
    assert {path.name: path.read_bytes() for path in directory.iterdir()} == before
    assert assets["sprite"]["frames"] == 2
    assert assets["sprite"]["colors"] == 2
    assert catalog(directory)["project"]["fx"]["assets"][0].get("anchor") is None


@pytest.mark.parametrize(
    "kind", ["palette", "timeline", "delivery", "path", "unknown_effect"]
)
def test_invalid_export(content: tuple[Path, dict], kind: str) -> None:
    directory, project = content
    if kind == "palette":
        project["fx"]["assets"][0]["id"] = 32
    elif kind == "timeline":
        project["timelines"]["timelines"][0]["events"][0]["tick"] = 1000
    elif kind == "delivery":
        project["deliveries"]["deliveries"][0]["speed"] = 0
    elif kind == "path":
        project["fx"]["assets"][0]["sheet"] = "../outside.png"
    else:
        project["deliveries"]["deliveries"][0]["impactEffect"] = "missing"
    with pytest.raises((ValueError, OSError)):
        validate_project(project, directory)


def test_http(content: tuple[Path, dict]) -> None:
    directory, project = content
    server = ThreadingHTTPServer(
        ("127.0.0.1", 0), partial(Handler, content=directory, token="test-token")
    )
    thread = Thread(target=server.serve_forever, daemon=True)
    thread.start()
    connection = HTTPConnection("127.0.0.1", server.server_port, timeout=10)
    try:
        connection.request("GET", "/")
        response = connection.getresponse()
        assert response.status == 200 and b"test-token" in response.read()
        connection.request("GET", "/api/catalog")
        response = connection.getresponse()
        assert json.loads(response.read())["assets"]["sprite"]["frames"] == 2
        connection.request(
            "POST",
            "/api/export",
            json.dumps(project),
            {"Content-Type": "application/json"},
        )
        response = connection.getresponse()
        assert response.status == 403
        response.read()
        connection.request(
            "POST",
            "/api/export",
            json.dumps(project),
            {"Content-Type": "application/json", "X-Editor-Token": "test-token"},
        )
        response = connection.getresponse()
        assert response.status == 200 and response.read().startswith(b"PK")
        connection.request("GET", "/api/catalog", headers={"Host": "example.invalid"})
        response = connection.getresponse()
        assert response.status == 403
        response.read()
        connection.request("GET", "/../skill_fx.py")
        response = connection.getresponse()
        assert response.status == 404
        response.read()
    finally:
        connection.close()
        server.shutdown()
        server.server_close()
        thread.join()


def test_playback_model() -> None:
    node = shutil.which("node")
    assert node, "Node.js is required for editor playback tests"
    module = (ROOT / "hacks/tools/effect_editor/model.mjs").as_uri()
    recipes = ROOT / "hacks/content/skill_deliveries.json"
    code = f"""
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {{frameAt,latestCue,simulate}} from {json.dumps(module)};
const defs=JSON.parse(fs.readFileSync({json.dumps(str(recipes))},'utf8')).deliveries;
assert.equal(frameAt(-1,3,2),null);
assert.equal(frameAt(2,3,2),0);
assert.equal(frameAt(3,3,2),1);
assert.equal(frameAt(6,3,2),null);
const timeline={{releaseTick:6,recoveryTicks:9,events:[{{tick:0,effect:'ward'}},{{tick:6,effect:'impact'}}]}};
assert.equal(latestCue(timeline,-1),null);
assert.equal(latestCue(timeline,5).effect,'ward');
assert.equal(latestCue(timeline,6).effect,'impact');
assert.equal(latestCue(timeline,15),null);
assert.equal(simulate(defs[0],-1).contacts,0);
assert.equal(simulate(defs[0],100).contacts,1);
assert.equal(simulate(defs[0],100).alive,false);
assert.equal(simulate(defs[1],100,3).contacts,3);
assert.equal(simulate(defs[4],100,1).contacts,3);
const burst=structuredClone(defs[3]);burst.maxTargets=2;
assert.equal(simulate(burst,100,3).contacts,2);
console.log('PASS: frame stepping, cue replacement, piercing and repeat budgets');
"""
    subprocess.run([node, "--input-type=module", "-e", code], check=True)


def test_browser_controls(content: tuple[Path, dict], tmp_path: Path) -> None:
    browser = shutil.which("chromium") or shutil.which("google-chrome")
    edge = Path("C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe")
    if browser is None and edge.exists():
        browser = str(edge)
    if browser is None:
        pytest.skip("Chromium or Edge is required for the browser smoke test")
    directory, _ = content
    harness = r"""<!doctype html><html><body><output id="result">RUNNING</output>
<iframe id="editor" src="/" style="width:1400px;height:1000px"></iframe><script>
(async()=>{
const frame=document.querySelector('iframe');
const wait=async test=>{for(let i=0;i<150;i++){if(test())return;await new Promise(r=>setTimeout(r,30));}throw Error('Timed out');};
const assert=(ok,message)=>{if(!ok)throw Error(message);};
await wait(()=>frame.contentDocument?.querySelector('#status')?.textContent.includes('Source loaded'));
const d=frame.contentDocument,w=frame.contentWindow;
const el=id=>d.getElementById(id);
const click=selector=>d.querySelector(selector).click();
const edit=(label,value)=>{const wrap=[...d.querySelectorAll('#fields label')].find(e=>e.firstChild.textContent===label);assert(wrap,'Missing field '+label);const input=wrap.querySelector('input,select');input.value=value;input.dispatchEvent(new w.Event('change',{bubbles:true}));};
click('#forward');assert(el('tick').textContent.includes('Tick 1'),'Frame step');
el('scrub').value=3;el('scrub').dispatchEvent(new w.Event('input'));assert(el('metrics').textContent.includes('frame 2/2'),'Frame scrub');
edit('Anchor X',2);edit('Frame ticks',2);assert(el('json').value.includes('"anchor"'),'Anchor edit');
click('button[data-mode="effects"]');
[...d.querySelectorAll('#fields button')].find(e=>e.textContent==='Add layer').click();
assert(d.querySelectorAll('.layer').length===2,'Add layer');
click('button[data-mode="timelines"]');edit('Release tick',9);assert(el('metrics').textContent.includes('Release 9t'),'Timing edit');
click('button[data-mode="deliveries"]');edit('radius',2);
el('cast').value='0';el('cast').dispatchEvent(new w.Event('change'));assert(el('timeline').querySelector('.release'),'Combined cast');
click('#play');assert(el('play').textContent==='Pause','Playback');click('#play');
click('#validate');await wait(()=>el('status').textContent.startsWith('Valid:')||el('status').className==='error');
assert(el('status').textContent.startsWith('Valid:'),el('status').textContent);
// Invalid advanced JSON must preserve the editable draft.
click('button[data-mode="timelines"]');const json=JSON.parse(el('json').value);json.events[0].effect='missing';el('json').value=JSON.stringify(json);click('#apply');
await wait(()=>el('status').className==='error');assert(el('metrics').textContent.includes('Release 9t'),'Invalid import changed draft');
document.getElementById('result').textContent='PASS: browser frame, anchor, layers, timing, delivery, validation and invalid JSON controls';
})().catch(error=>document.getElementById('result').textContent='FAIL: '+error.message);
</script></body></html>"""

    class BrowserHandler(Handler):
        def do_GET(self) -> None:
            if self.path == "/test":
                self.send(harness.encode(), "text/html; charset=utf-8")
            else:
                super().do_GET()

    server = ThreadingHTTPServer(
        ("127.0.0.1", 0),
        partial(BrowserHandler, content=directory, token="browser-test"),
    )
    thread = Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        result = subprocess.run(
            [
                browser,
                "--headless=new",
                "--no-first-run",
                "--disable-gpu",
                "--disable-background-networking",
                "--no-default-browser-check",
                f"--user-data-dir={tmp_path / 'browser'}",
                "--virtual-time-budget=15000",
                "--dump-dom",
                f"http://127.0.0.1:{server.server_port}/test",
            ],
            capture_output=True,
            timeout=45,
        )
        output = result.stdout.decode("utf-8", errors="replace")
        assert '<output id="result">PASS:' in output, output + result.stderr.decode(
            errors="replace"
        )
    finally:
        server.shutdown()
        server.server_close()
        thread.join()
