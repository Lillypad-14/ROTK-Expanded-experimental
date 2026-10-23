; Ringwraith gear: wraith-only drops, orange unique names and the wraith-only equip check.
.if HACK_skill_expansion
hookReplaceR3 loot_generateItem, wraithGear_generateItem
hookReplace item_affix_formatName, wraithGear_formatName
hookReplace item_getEquipSlot, wraithGear_getEquipSlot

; Direct ItemSuffixFlags reads index wraith suffixes past the stock table.
.org 0x08026c40
    .word WraithItemSuffixFlags
.org 0x08026f4c
    .word WraithItemSuffixFlags
.org 0x080383dc
    .word WraithItemSuffixFlags
.endif
