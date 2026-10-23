; Ringwraith gear tables: copy the stock rows into the head of each extended Wraith* table and
; point the wraith item types at them. Must follow the modcode incbin, which it overwrites.
.if HACK_skill_expansion

ITEM_STAT_RECORD_SIZE equ 12
ITEM_GFX_SIZE equ 28
ITEM_METADATA_SIZE equ 8
AFFIX_FLAGS_SIZE equ 4
ITEM_TYPE_INFO_SIZE equ 40

.macro wraithCopy, dst, src, count, size
    .org dst
    .incbin "rotk.gba", src - 0x08000000, (count) * (size)
.endmacro

.macro wraithBaseTables, items, gfx, flags, stockItems, stockGfx, stockFlags, count
    wraithCopy items, stockItems, count, ITEM_STAT_RECORD_SIZE
    wraithCopy gfx, stockGfx, count, ITEM_GFX_SIZE
    wraithCopy flags, stockFlags, count, ITEM_METADATA_SIZE
.endmacro

.macro wraithPrefixTables, prefixes, flags, stockPrefixes, stockFlags, count
    wraithCopy prefixes, stockPrefixes, count, ITEM_STAT_RECORD_SIZE
    wraithCopy flags, stockFlags, count, AFFIX_FLAGS_SIZE
.endmacro

; ItemTypeInfo fields: baseItems 0, prefixes 8, prefixFlags 12, suffixes 20, suffixFlags 24,
; baseGfx 28, baseMetadata 32.
.macro wraithRepointType, itemType, items, gfx, flags, prefixes, prefixFlags
    .org ItemTypeInfos + (itemType) * ITEM_TYPE_INFO_SIZE
    .word items
    .org ItemTypeInfos + (itemType) * ITEM_TYPE_INFO_SIZE + 8
    .word prefixes
    .word prefixFlags
    .org ItemTypeInfos + (itemType) * ITEM_TYPE_INFO_SIZE + 20
    .word WraithItemSuffixes
    .word WraithItemSuffixFlags
    .word gfx
    .word flags
.endmacro

wraithBaseTables WraithMaceItems, WraithMaceGfx, WraithMaceItemFlags, MaceItems, MaceGfx, MaceItemFlags, 10
wraithBaseTables WraithSword1HItems, WraithSword1HGfx, WraithSword1HItemFlags, Sword1HItems, Sword1HGfx, Sword1HItemFlags, 11
wraithBaseTables WraithArmorItems, WraithArmorGfx, WraithArmorItemFlags, ArmorItems, ArmorGfx, ArmorItemFlags, 24
wraithBaseTables WraithAccessoryItems, WraithAccessoryGfx, WraithAccessoryItemFlags, AccessoryItems, AccessoryGfx, AccessoryItemFlags, 12
wraithBaseTables WraithCloakItems, WraithCloakGfx, WraithCloakItemFlags, CloakItems, CloakGfx, CloakItemFlags, 9
wraithBaseTables WraithGloveItems, WraithGloveGfx, WraithGloveItemFlags, GloveItems, GloveGfx, GloveItemFlags, 8
wraithBaseTables WraithShoeItems, WraithShoeGfx, WraithShoeItemFlags, ShoeItems, ShoeGfx, ShoeItemFlags, 7

wraithPrefixTables WraithWeaponPrefixes, WraithWeaponPrefixFlags, WeaponPrefixes, WeaponPrefixFlags, 156
wraithPrefixTables WraithArmorPrefixes, WraithArmorPrefixFlags, ArmorPrefixes, ArmorPrefixFlags, 156
wraithPrefixTables WraithAccessoryPrefixes, WraithAccessoryPrefixFlags, AccessoryPrefixes, AccessoryPrefixFlags, 156
wraithPrefixTables WraithCloakPrefixes, WraithCloakPrefixFlags, CloakPrefixes, CloakPrefixFlags, 36
wraithPrefixTables WraithGloveShoePrefixes, WraithGloveShoePrefixFlags, GloveShoePrefixes, GloveShoePrefixFlags, 96

wraithCopy WraithItemSuffixes, ItemSuffixes, 160, ITEM_STAT_RECORD_SIZE
wraithCopy WraithItemSuffixFlags, ItemSuffixFlags, 160, AFFIX_FLAGS_SIZE

wraithRepointType 1, WraithSword1HItems, WraithSword1HGfx, WraithSword1HItemFlags, WraithWeaponPrefixes, WraithWeaponPrefixFlags
wraithRepointType 4, WraithMaceItems, WraithMaceGfx, WraithMaceItemFlags, WraithWeaponPrefixes, WraithWeaponPrefixFlags
wraithRepointType 8, WraithArmorItems, WraithArmorGfx, WraithArmorItemFlags, WraithArmorPrefixes, WraithArmorPrefixFlags
wraithRepointType 10, WraithAccessoryItems, WraithAccessoryGfx, WraithAccessoryItemFlags, WraithAccessoryPrefixes, WraithAccessoryPrefixFlags
wraithRepointType 11, WraithCloakItems, WraithCloakGfx, WraithCloakItemFlags, WraithCloakPrefixes, WraithCloakPrefixFlags
wraithRepointType 12, WraithGloveItems, WraithGloveGfx, WraithGloveItemFlags, WraithGloveShoePrefixes, WraithGloveShoePrefixFlags
wraithRepointType 13, WraithShoeItems, WraithShoeGfx, WraithShoeItemFlags, WraithGloveShoePrefixes, WraithGloveShoePrefixFlags

.endif
