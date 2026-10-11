from pathlib import Path

import pytest

from rotkit.ghidra_build.apply_symbols import _strip_type_qualifiers
from rotkit.ghidra_build.test_parse_headers import run_ghidra_test


@pytest.mark.parametrize(
    ("type_name", "expected"),
    [
        ("const TextPackage *", "TextPackage *"),
        ("const s32 *", "s32 *"),
        ("const HuffmanNode *", "HuffmanNode *"),
        ("volatile u16", "u16"),
        ("u8 *const", "u8 *"),
        ("void *const [8]", "void * [8]"),
        ("const volatile u32", "u32"),
        ("volatile const u32", "u32"),
        ("u32 const volatile", "u32"),
        ("const u8 *const *volatile [2]", "u8 * * [2]"),
        ("  const\tu32  *  ", "u32 *"),
        ("unsigned int", "unsigned int"),
        ("ConstantInfo *", "ConstantInfo *"),
        ("constant *", "constant *"),
        ("volatileState *", "volatileState *"),
    ],
)
def test_strip_type_qualifiers(type_name: str, expected: str) -> None:
    assert _strip_type_qualifiers(type_name) == expected


def test_resolve_type_array_dimensions(tmp_path: Path) -> None:
    run_ghidra_test(
        tmp_path,
        """
import pyghidra
pyghidra.start()
from ghidra.program.model.data import Array, Enum, Pointer, StandAloneDataTypeManager
from ghidra.util.task import ConsoleTaskMonitor
from rotkit.ghidra_build.apply_symbols import _resolve_type
from rotkit.ghidra_build.parse_headers import parse_into

manager = StandAloneDataTypeManager("store-array-test")
transaction = manager.startTransaction("parse")
try:
    parse_into(manager, ConsoleTaskMonitor())
    typemap = {}
    enum_values = {}
    for dt in manager.getAllDataTypes():
        typemap.setdefault(str(dt.getName()), dt)
        if isinstance(dt, Enum):
            for name in dt.getNames():
                enum_values.setdefault(str(name), int(dt.getValue(name)))

    errors = []
    for type_name, dimensions in (
        ("s8[35][6]", [(35, 6), (6, 1)]),
        ("s8[8][6][6]", [(8, 36), (6, 6), (6, 1)]),
        (
            "SpriteAnimation *const [ENEMY_TYPE_COUNT][ENEMY_ANIMATION_POSE_COUNT]",
            [(83, 24), (6, 4)],
        ),
        ("const u8 *const *volatile [2][3]", [(2, 12), (3, 4)]),
        ("u16 [0x2] [0x3]", [(2, 6), (3, 2)]),
        ("const TextPackage *[5]", [(5, 4)]),
        ("u8[4]", [(4, 1)]),
    ):
        dt = _resolve_type(type_name, typemap, enum_values, errors)
        assert dt.getLength() == dimensions[0][0] * dimensions[0][1], type_name
        for count, stride in dimensions:
            assert isinstance(dt, Array), type_name
            actual = (dt.getNumElements(), dt.getElementLength())
            assert actual == (count, stride), (type_name, actual, (count, stride))
            dt = dt.getDataType()
        assert not isinstance(dt, Array), type_name
        if "*" in type_name:
            assert isinstance(dt, Pointer), type_name

    for type_name in ("", "u8[]", "s8[][6]", "s8[2][]"):
        assert _resolve_type(type_name, typemap, enum_values, errors) is None, type_name
    assert errors == [], errors

    assert _resolve_type("s8[2][MISSING]", typemap, enum_values, errors) is None
    assert errors == ["array length 'MISSING' is not an integer or enum value"], errors
    errors.clear()
    assert _resolve_type("MissingType[2]", typemap, enum_values, errors) is None
    assert errors == ["data type 'MissingType' not found in DTM"], errors
finally:
    manager.endTransaction(transaction, False)
    manager.close()
""",
    )
