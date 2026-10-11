import pytest

from rotkit.ghidra_build.apply_symbols import _strip_type_qualifiers


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
