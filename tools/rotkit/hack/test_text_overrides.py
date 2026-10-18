"""Host-C tests for text override validation, lookup, and generated literals."""

import ctypes
from pathlib import Path
import shutil
import subprocess

import pytest

from rotkit.hack.text_overrides import discover_sources, load_overrides, render_include
from rotkit.paths import ROOT


@pytest.fixture
def compiler() -> str:
    compiler = shutil.which("cc") or shutil.which("gcc")
    if compiler is None:
        pytest.skip("needs a host C compiler")
    return compiler


def compile_config(
    tmp_path: Path, compiler: str, config: str, shared: bool = False
) -> subprocess.CompletedProcess[str]:
    patch = ROOT / "hacks/patches/text_overrides"
    source = tmp_path / "text_overrides.c"
    source.write_text((patch / "text_overrides.c").read_text())
    (tmp_path / "text_overrides.inc").write_text(config)
    (tmp_path / "variables.h").write_text(
        '#include "text.h"\n'
        "extern const TextPackage *g_TextPackage;\n"
        "extern const HuffmanNode *g_TextHuffmanTree;\n"
        "extern const s32 *g_TextStringOffsets;\n"
    )
    command = [
        compiler,
        "-std=gnu11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-DHACK_text_overrides=1",
        f"-I{tmp_path}",
        f"-I{patch}",
        f"-I{ROOT / 'include'}",
        f"-I{ROOT / 'carved/include'}",
    ]
    if shared:
        stubs = tmp_path / "stubs.c"
        stubs.write_text(
            '#include "text.h"\n'
            "static u8 package[4];\n"
            "const TextPackage *g_TextPackage = (const TextPackage *)package;\n"
            "const HuffmanNode *g_TextHuffmanTree;\n"
            "const s32 *g_TextStringOffsets;\n"
            "u8 language;\n"
            "u8 text_getLanguage(void) { return language; }\n"
        )
        command.extend(
            (
                "-shared",
                "-fPIC",
                str(source),
                str(stubs),
                "-o",
                str(tmp_path / "overrides.so"),
            )
        )
    else:
        command.extend(("-fsyntax-only", str(source)))
    return subprocess.run(command, capture_output=True, text=True)


@pytest.mark.parametrize(
    "config",
    (
        "",
        'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "")',
        r'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "Pr\x9e" "zision\n@1")',
        'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "one")\nTEXT_OVERRIDE(LANGUAGE_EN, TEXT_ID_FRODO, "two")',
        'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "' + "x" * 1023 + '")',
    ),
)
def test_valid_declarations(tmp_path: Path, compiler: str, config: str) -> None:
    result = compile_config(tmp_path, compiler, config)
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize(
    "config",
    (
        'TEXT_OVERRIDE(5, TEXT_ID_FRODO, "text")',
        'TEXT_OVERRIDE(-1, TEXT_ID_FRODO, "text")',
        'TEXT_OVERRIDE(LANGUAGE_DE, 65536, "text")',
        'TEXT_OVERRIDE(LANGUAGE_DE, -1, "text")',
        'TEXT_OVERRIDE(LANGUAGE_DE, MISSING_ID, "text")',
        'TEXT_OVERRIDE(MISSING_LANGUAGE, TEXT_ID_FRODO, "text")',
        'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "one")\nTEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "two")',
        'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "one")\nTEXT_OVERRIDE(LANGUAGE_DE, 19, "two")',
        'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "' + "x" * 1024 + '")',
    ),
)
def test_invalid_declarations(tmp_path: Path, compiler: str, config: str) -> None:
    assert compile_config(tmp_path, compiler, config).returncode != 0


def test_macro_lookup_and_encoded_literals(tmp_path: Path, compiler: str) -> None:
    config = (
        r'TEXT_OVERRIDE(LANGUAGE_DE, TEXT_ID_FRODO, "Pr\x9e" "zision\n@1")' + "\n"
        'TEXT_OVERRIDE(LANGUAGE_EN, TEXT_ID_FRODO, "")\n'
        'TEXT_OVERRIDE(LANGUAGE_EN, 461, "Rune override")\n'
    )
    result = compile_config(tmp_path, compiler, config, shared=True)
    assert result.returncode == 0, result.stderr
    library = ctypes.CDLL(str(tmp_path / "overrides.so"))
    language = ctypes.c_uint8.in_dll(library, "language")
    decode = library.textOverrides_decodeHuffmanString
    decode.argtypes = (ctypes.c_int32, ctypes.c_void_p, ctypes.c_int32)
    decode.restype = ctypes.c_int
    buffer = ctypes.create_string_buffer(100)
    language.value = 2
    assert decode(19, buffer, 100) == 0
    assert buffer.value == b"Pr\x9ezision\n@1"
    language.value = 0
    assert decode(19, buffer, 1) == 0
    assert buffer.value == b""
    assert decode(461, buffer, 100) == 0
    assert buffer.value == b"Rune override"


def test_generated_toml_decoder(tmp_path: Path, compiler: str) -> None:
    config = tmp_path / "strings.text-override.toml"
    config.write_text(
        '[TEXT_ID_FRODO]\nde = "Präzision\\n@1 \\"Zitat\\" \\\\ ??/"\nen = ""\n',
        encoding="utf-8",
    )
    result = compile_config(
        tmp_path, compiler, render_include(load_overrides([config])), shared=True
    )
    assert result.returncode == 0, result.stderr
    library = ctypes.CDLL(str(tmp_path / "overrides.so"))
    decode = library.textOverrides_decodeHuffmanString
    decode.argtypes = (ctypes.c_int32, ctypes.c_void_p, ctypes.c_int32)
    decode.restype = ctypes.c_int
    language = ctypes.c_uint8.in_dll(library, "language")
    buffer = ctypes.create_string_buffer(100)
    language.value = 2
    assert decode(19, buffer, 100) == 0
    assert buffer.value == b'Pr\x9ezision\n@1 "Zitat" \\ ??/'
    language.value = 0
    assert decode(19, buffer, 100) == 0
    assert buffer.value == b""


def test_default_config_compiles(tmp_path: Path, compiler: str) -> None:
    overrides = load_overrides(discover_sources())
    result = compile_config(tmp_path, compiler, render_include(overrides))
    assert result.returncode == 0, result.stderr
