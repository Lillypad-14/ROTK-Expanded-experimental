#if HACK_skill_expansion

#include "patches/ringwraith/wraith_gear.h"
#include "patches/ringwraith/ringwraith.h"
#include "gba_io.h"
#include "loot.h"
#include "memory.h"
#include "rng.h"
#include "stats.h"
#include "text.h"
#include "variables.h"

#define WRAITH_TEXT_ID_BASE 0x4000
#define WRAITH_TIER_COUNT 5
#define WRAITH_NO_UNIQUE 0xff
#define WRAITH_SUFFIX_STOCK_COUNT ARRAY_COUNT(ItemSuffixes)

// Orange ramp for unique names, brightest first, written into a free BG palette bank.
#define ORANGE_COLOR_INDEX 6
#define ORANGE_BRIGHT 0x0a5f
#define ORANGE_MID 0x05b9
#define ORANGE_DARK 0x0113

#define WRAITH_STRINGS(X)                                                                          \
    X(MACE_1, "Grave Mace")                                                                        \
    X(MACE_2, "Barrow Mace")                                                                       \
    X(MACE_3, "Morgul Mace")                                                                       \
    X(MACE_4, "Dread Mace")                                                                        \
    X(MACE_5, "Black Mace")                                                                        \
    X(MACE_UNIQUE, "Mace of the Witch-king")                                                       \
    X(BLADE_1, "Grave Blade")                                                                      \
    X(BLADE_2, "Barrow Blade")                                                                     \
    X(BLADE_3, "Morgul Blade")                                                                     \
    X(BLADE_4, "Dread Blade")                                                                      \
    X(BLADE_5, "Black Blade")                                                                      \
    X(BLADE_UNIQUE, "The Morgul-Knife")                                                            \
    X(CROWN_1, "Grave Crown")                                                                      \
    X(CROWN_2, "Barrow Crown")                                                                     \
    X(CROWN_3, "Morgul Crown")                                                                     \
    X(CROWN_4, "Dread Crown")                                                                      \
    X(CROWN_5, "Black Crown")                                                                      \
    X(HAUBERK_1, "Grave Hauberk")                                                                  \
    X(HAUBERK_2, "Barrow Hauberk")                                                                 \
    X(HAUBERK_3, "Morgul Hauberk")                                                                 \
    X(HAUBERK_4, "Dread Hauberk")                                                                  \
    X(HAUBERK_5, "Black Hauberk")                                                                  \
    X(CROWN_UNIQUE, "Iron Crown of Angmar")                                                        \
    X(HAUBERK_UNIQUE, "Barrow-wight Hauberk")                                                      \
    X(RING_1, "Grave Ring")                                                                        \
    X(RING_2, "Barrow Ring")                                                                       \
    X(RING_3, "Morgul Ring")                                                                       \
    X(RING_4, "Dread Ring")                                                                        \
    X(RING_5, "Black Ring")                                                                        \
    X(RING_UNIQUE, "The Ninth Ring")                                                               \
    X(SHROUD_1, "Grave Shroud")                                                                    \
    X(SHROUD_2, "Barrow Shroud")                                                                   \
    X(SHROUD_3, "Morgul Shroud")                                                                   \
    X(SHROUD_4, "Dread Shroud")                                                                    \
    X(SHROUD_5, "Black Shroud")                                                                    \
    X(SHROUD_UNIQUE, "Shroud of Unbeing")                                                          \
    X(GAUNTLETS_1, "Grave Gauntlets")                                                              \
    X(GAUNTLETS_2, "Barrow Gauntlets")                                                             \
    X(GAUNTLETS_3, "Morgul Gauntlets")                                                             \
    X(GAUNTLETS_4, "Dread Gauntlets")                                                              \
    X(GAUNTLETS_5, "Black Gauntlets")                                                              \
    X(GAUNTLETS_UNIQUE, "Gauntlets of the Black Captain")                                          \
    X(GREAVES_1, "Grave Greaves")                                                                  \
    X(GREAVES_2, "Barrow Greaves")                                                                 \
    X(GREAVES_3, "Morgul Greaves")                                                                 \
    X(GREAVES_4, "Dread Greaves")                                                                  \
    X(GREAVES_5, "Black Greaves")                                                                  \
    X(GREAVES_UNIQUE, "Greaves of the Morgul Vale")                                                \
    X(PREFIX_DREADFUL, "Dreadful")                                                                 \
    X(PREFIX_COLD, "Cold")                                                                         \
    X(PREFIX_HUNGERING, "Hungering")                                                               \
    X(PREFIX_SHADOWED, "Shadowed")                                                                 \
    X(PREFIX_WITHERING, "Withering")                                                               \
    X(PREFIX_GRAVE_COLD, "Grave-cold")                                                             \
    X(PREFIX_TERRIBLE, "Terrible")                                                                 \
    X(PREFIX_COMMANDING, "Commanding")                                                             \
    X(PREFIX_RAVENOUS, "Ravenous")                                                                 \
    X(PREFIX_SORCEROUS, "Sorcerous")                                                               \
    X(PREFIX_PETRIFYING, "Petrifying")                                                             \
    X(PREFIX_DEATHLESS, "Deathless")                                                               \
    X(PREFIX_SOUL_EATING, "Soul-eating")                                                           \
    X(PREFIX_UNDYING, "Undying")                                                                   \
    X(PREFIX_ABYSSAL, "Abyssal")                                                                   \
    X(PREFIX_NAMELESS, "Nameless")                                                                 \
    X(SUFFIX_NINE, "of the Nine")                                                                  \
    X(SUFFIX_BARROWS, "of the Barrows")                                                            \
    X(SUFFIX_SHADOW, "of Shadow")                                                                  \
    X(SUFFIX_COLD_IRON, "of Cold Iron")                                                            \
    X(SUFFIX_HUNGER, "of Hunger")                                                                  \
    X(SUFFIX_THRALLDOM, "of Thralldom")                                                            \
    X(SUFFIX_ANGMAR, "of Angmar")                                                                  \
    X(SUFFIX_DESPAIR, "of Despair")                                                                \
    X(SUFFIX_DOL_GULDUR, "of Dol Guldur")                                                          \
    X(SUFFIX_MINAS_MORGUL, "of Minas Morgul")                                                      \
    X(SUFFIX_WITCH_KING, "of the Witch-king")                                                      \
    X(SUFFIX_DARK_LORD, "of the Dark Lord")                                                        \
    X(MAUL_1, "Wight Maul")                                                                        \
    X(MAUL_2, "Carn Dum Maul")                                                                     \
    X(MAUL_3, "Angmar Maul")                                                                       \
    X(MAUL_4, "Udun Maul")                                                                         \
    X(MAUL_5, "Nazgul Maul")                                                                       \
    X(FANG_1, "Wight Fang")                                                                        \
    X(FANG_2, "Carn Dum Fang")                                                                     \
    X(FANG_3, "Angmar Fang")                                                                       \
    X(FANG_4, "Udun Fang")                                                                         \
    X(FANG_5, "Nazgul Fang")                                                                       \
    X(HELM_1, "Wight Helm")                                                                        \
    X(HELM_2, "Carn Dum Helm")                                                                     \
    X(HELM_3, "Angmar Helm")                                                                       \
    X(HELM_4, "Udun Helm")                                                                         \
    X(HELM_5, "Nazgul Helm")                                                                       \
    X(MAIL_1, "Wight Mail")                                                                        \
    X(MAIL_2, "Carn Dum Mail")                                                                     \
    X(MAIL_3, "Angmar Mail")                                                                       \
    X(MAIL_4, "Udun Mail")                                                                         \
    X(MAIL_5, "Nazgul Mail")                                                                       \
    X(PENDANT_1, "Wight Pendant")                                                                  \
    X(PENDANT_2, "Carn Dum Pendant")                                                               \
    X(PENDANT_3, "Angmar Pendant")                                                                 \
    X(PENDANT_4, "Udun Pendant")                                                                   \
    X(PENDANT_5, "Nazgul Pendant")                                                                 \
    X(WRAP_1, "Wight Wrap")                                                                        \
    X(WRAP_2, "Carn Dum Wrap")                                                                     \
    X(WRAP_3, "Angmar Wrap")                                                                       \
    X(WRAP_4, "Udun Wrap")                                                                         \
    X(WRAP_5, "Nazgul Wrap")                                                                       \
    X(GRIPS_1, "Wight Grips")                                                                      \
    X(GRIPS_2, "Carn Dum Grips")                                                                   \
    X(GRIPS_3, "Angmar Grips")                                                                     \
    X(GRIPS_4, "Udun Grips")                                                                       \
    X(GRIPS_5, "Nazgul Grips")                                                                     \
    X(TREADS_1, "Wight Treads")                                                                    \
    X(TREADS_2, "Carn Dum Treads")                                                                 \
    X(TREADS_3, "Angmar Treads")                                                                   \
    X(TREADS_4, "Udun Treads")                                                                     \
    X(TREADS_5, "Nazgul Treads")                                                                   \
    X(BANNER_UNIQUE, "Banner of Angmar")                                                           \
    X(DART_UNIQUE, "The Black Dart")                                                               \
    X(BARROW_CROWN_UNIQUE, "Crown of the Barrow-king")                                             \
    X(MASK_UNIQUE, "Mask of Dol Guldur")                                                           \
    X(SIGNET_UNIQUE, "Carn Dum Signet")                                                            \
    X(HORN_UNIQUE, "Horn of Minas Morgul")                                                         \
    X(KHAMUL_UNIQUE, "Ring of Khamul")                                                             \
    X(VOICE_UNIQUE, "Voice of Sauron")                                                             \
    X(MARSHES_UNIQUE, "Shroud of the Dead Marshes")                                                \
    X(UNLIT_UNIQUE, "The Unlit Hand")                                                              \
    X(SADDLE_UNIQUE, "Fell Rider's Spurs")                                                         \
    X(PREFIX_GRAVEBOUND, "Gravebound")                                                             \
    X(PREFIX_LURKING, "Lurking")                                                                   \
    X(PREFIX_HARROWING, "Harrowing")                                                               \
    X(PREFIX_NECROMANTIC, "Necromantic")                                                           \
    X(SUFFIX_GRAVE, "of the Grave")                                                                \
    X(SUFFIX_UNSEEN, "of the Unseen")                                                              \
    X(SUFFIX_TERROR, "of Terror")                                                                  \
    X(SUFFIX_HOST, "of the Host")

enum WraithTextId
{
#define X(id, text) WRAITH_TEXT_##id,
    WRAITH_STRINGS(X)
#undef X
    WRAITH_TEXT_COUNT,
};

static const char *const WraithStrings[WRAITH_TEXT_COUNT] = {
#define X(id, text) text,
    WRAITH_STRINGS(X)
#undef X
};

#define RECORD(text, tier_, s0, v0, s1, v1, s2, v2)                                                \
    {.nameId = WRAITH_TEXT_ID_BASE + WRAITH_TEXT_##text,                                           \
     .tier = (tier_),                                                                              \
     .stat0 = (s0),                                                                                \
     .val0 = (v0),                                                                                 \
     .stat1 = (s1),                                                                                \
     .val1 = (v1),                                                                                 \
     .stat2 = (s2),                                                                                \
     .val2 = (v2),                                                                                 \
     .stat3 = STAT_NONE,                                                                           \
     .val3 = 0}
#define RECORD2(text, tier_, s0, v0, s1, v1) RECORD(text, tier_, s0, v0, s1, v1, STAT_NONE, 0)
#define RECORD1(text, tier_, s0, v0) RECORD(text, tier_, s0, v0, STAT_NONE, 0, STAT_NONE, 0)
#define RECORD0(text, tier_) RECORD(text, tier_, STAT_NONE, 0, STAT_NONE, 0, STAT_NONE, 0)

#define WRAITH_CLASS ITEM_FLAG_CLASS_BIT(RINGWRAITH_BASE_HERO)
#define WRAITH_UNIQUE_FLAGS (ITEM_FLAG_NO_PREFIX | ITEM_FLAG_NO_SUFFIX)
#define META(flags_, sfx) {.flags.p = (flags_) | WRAITH_CLASS, .equipSfx = (sfx)}
#define META5(flags_, sfx) META(flags_, sfx), META(flags_, sfx), META(flags_, sfx), META(flags_, sfx), META(flags_, sfx)

#define MACE_FLAGS (ITEM_FAMILY_MACE | ITEM_FLAG_SLOT_MAINHAND | ITEM_FLAG_SLOT_OFFHAND)
#define BLADE_FLAGS (ITEM_FAMILY_SWORD | ITEM_FLAG_SLOT_MAINHAND | ITEM_FLAG_SLOT_OFFHAND)

// Wraith base items: runs of five tiers, with uniques after them. See WraithDropSlots.
#define MACE_STOCK ARRAY_COUNT(MaceItems)
#define BLADE_STOCK ARRAY_COUNT(Sword1HItems)
#define ARMOR_STOCK ARRAY_COUNT(ArmorItems)
#define RING_STOCK ARRAY_COUNT(AccessoryItems)
#define SHROUD_STOCK ARRAY_COUNT(CloakItems)
#define GAUNTLETS_STOCK ARRAY_COUNT(GloveItems)
#define GREAVES_STOCK ARRAY_COUNT(ShoeItems)

// Items save their base index, so new bases are only ever appended.
#define MACE_COUNT 12
#define BLADE_COUNT 12
#define ARMOR_COUNT 24
#define RING_COUNT 15
#define SHROUD_COUNT 12
#define GAUNTLETS_COUNT 12
#define GREAVES_COUNT 12

// The stock entries of every Wraith* table are copied in from the ROM by wraith_gear_tables.asm.
const ItemStatRecord WraithMaceItems[] = {
    [MACE_STOCK] = RECORD2(MACE_1, 1, STAT_DAMAGE_SLASH, 7, STAT_CRIT_CHANCE_PERCENT, 5),
    RECORD2(MACE_2, 3, STAT_DAMAGE_SLASH, 12, STAT_CRIT_CHANCE_PERCENT, 10),
    RECORD2(MACE_3, 5, STAT_DAMAGE_SLASH, 17, STAT_CRIT_CHANCE_PERCENT, 10),
    RECORD2(MACE_4, 7, STAT_DAMAGE_SLASH, 22, STAT_CRIT_CHANCE_PERCENT, 15),
    RECORD2(MACE_5, 9, STAT_DAMAGE_SLASH, 28, STAT_CRIT_CHANCE_PERCENT, 20),
    RECORD(MACE_UNIQUE, 6, STAT_DAMAGE_SLASH, 30, STAT_CRIT_CHANCE_PERCENT, 20, STAT_MAX_SPIRIT, -20),
    RECORD2(MAUL_1, 1, STAT_DAMAGE_SLASH, 8, STAT_COURAGE, 1),
    RECORD2(MAUL_2, 3, STAT_DAMAGE_SLASH, 13, STAT_COURAGE, 2),
    RECORD2(MAUL_3, 5, STAT_DAMAGE_SLASH, 18, STAT_COURAGE, 3),
    RECORD2(MAUL_4, 7, STAT_DAMAGE_SLASH, 23, STAT_COURAGE, 4),
    RECORD2(MAUL_5, 9, STAT_DAMAGE_SLASH, 29, STAT_COURAGE, 5),
    RECORD2(BANNER_UNIQUE, 7, STAT_DAMAGE_SLASH, 24, STAT_MAX_SPIRIT, 15),
};
const ItemGfx WraithMaceGfx[] = {
    [MACE_STOCK] = ITEM_GFX(MaceClub, LOOT_TYPE_CLUB),
    ITEM_GFX(MaceFlangedMace, LOOT_TYPE_MACE),
    ITEM_GFX(MaceSpikedMace, LOOT_TYPE_MACE),
    ITEM_GFX(MaceWarhammer, LOOT_TYPE_MACE),
    ITEM_GFX(MacePick, LOOT_TYPE_MACE),
    ITEM_GFX(UniqueGreenMaceOfTheOutlander, LOOT_TYPE_MACE),
    ITEM_GFX(MaceHeavyClub, LOOT_TYPE_CLUB),
    ITEM_GFX(MaceMace, LOOT_TYPE_MACE),
    ITEM_GFX(MaceLongMace, LOOT_TYPE_MACE),
    ITEM_GFX(MaceMattock, LOOT_TYPE_MACE),
    ITEM_GFX(MaceMaul, LOOT_TYPE_MACE),
    ITEM_GFX(UniqueOakstaffOfOldThalcos, LOOT_TYPE_MACE),
};
const ItemMetadata WraithMaceItemFlags[] = {
    [MACE_STOCK] = META5(MACE_FLAGS, 0x23),
    META(MACE_FLAGS | WRAITH_UNIQUE_FLAGS, 0x23),
    META5(MACE_FLAGS, 0x23),
    META(MACE_FLAGS | WRAITH_UNIQUE_FLAGS, 0x23),
};

const ItemStatRecord WraithSword1HItems[] = {
    [BLADE_STOCK] = RECORD2(BLADE_1, 1, STAT_DAMAGE_SLASH, 6, STAT_CRIT_CHANCE_PERCENT, 10),
    RECORD2(BLADE_2, 3, STAT_DAMAGE_SLASH, 10, STAT_CRIT_CHANCE_PERCENT, 12),
    RECORD2(BLADE_3, 5, STAT_DAMAGE_SLASH, 14, STAT_CRIT_CHANCE_PERCENT, 15),
    RECORD2(BLADE_4, 7, STAT_DAMAGE_SLASH, 19, STAT_CRIT_CHANCE_PERCENT, 18),
    RECORD2(BLADE_5, 9, STAT_DAMAGE_SLASH, 24, STAT_CRIT_CHANCE_PERCENT, 22),
    RECORD2(BLADE_UNIQUE, 6, STAT_DAMAGE_SLASH, 14, STAT_CRIT_CHANCE_PERCENT, 25),
    RECORD2(FANG_1, 1, STAT_DAMAGE_SLASH, 5, STAT_DODGE_PERCENT, 2),
    RECORD2(FANG_2, 3, STAT_DAMAGE_SLASH, 9, STAT_DODGE_PERCENT, 3),
    RECORD2(FANG_3, 5, STAT_DAMAGE_SLASH, 13, STAT_DODGE_PERCENT, 4),
    RECORD2(FANG_4, 7, STAT_DAMAGE_SLASH, 17, STAT_DODGE_PERCENT, 5),
    RECORD2(FANG_5, 9, STAT_DAMAGE_SLASH, 22, STAT_DODGE_PERCENT, 6),
    RECORD2(DART_UNIQUE, 7, STAT_DAMAGE_SLASH, 18, STAT_CRIT_CHANCE_PERCENT, 20),
};
const ItemGfx WraithSword1HGfx[] = {
    [BLADE_STOCK] = ITEM_GFX(Sword1HEstoc, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HScimitar, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HFalchion, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HSerratedSword, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HPrestigeSword, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HSwordRapier, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HBacksword, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HLeafSword, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HBroadsword, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HLongsword, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(Sword1HSteelsword, LOOT_TYPE_SWORD_ONE_HAND),
    ITEM_GFX(UniqueSting, LOOT_TYPE_SWORD_ONE_HAND),
};
const ItemMetadata WraithSword1HItemFlags[] = {
    [BLADE_STOCK] = META5(BLADE_FLAGS, 0x27),
    META(BLADE_FLAGS | WRAITH_UNIQUE_FLAGS, 0x27),
    META5(BLADE_FLAGS, 0x27),
    META(BLADE_FLAGS | WRAITH_UNIQUE_FLAGS, 0x27),
};

const ItemStatRecord WraithArmorItems[] = {
    [ARMOR_STOCK] = RECORD2(CROWN_1, 1, STAT_MELEE_ARMOR, 1, STAT_CRITICAL_ARMOR, 3),
    RECORD2(CROWN_2, 3, STAT_MELEE_ARMOR, 2, STAT_CRITICAL_ARMOR, 6),
    RECORD2(CROWN_3, 5, STAT_MELEE_ARMOR, 3, STAT_CRITICAL_ARMOR, 10),
    RECORD2(CROWN_4, 7, STAT_MELEE_ARMOR, 4, STAT_CRITICAL_ARMOR, 14),
    RECORD2(CROWN_5, 9, STAT_MELEE_ARMOR, 5, STAT_CRITICAL_ARMOR, 20),
    RECORD2(HAUBERK_1, 1, STAT_MELEE_ARMOR, 3, STAT_MISSILE_ARMOR, 2),
    RECORD2(HAUBERK_2, 3, STAT_MELEE_ARMOR, 6, STAT_MISSILE_ARMOR, 4),
    RECORD2(HAUBERK_3, 5, STAT_MELEE_ARMOR, 9, STAT_MISSILE_ARMOR, 7),
    RECORD2(HAUBERK_4, 7, STAT_MELEE_ARMOR, 13, STAT_MISSILE_ARMOR, 10),
    RECORD2(HAUBERK_5, 9, STAT_MELEE_ARMOR, 17, STAT_MISSILE_ARMOR, 13),
    RECORD(CROWN_UNIQUE, 7, STAT_MELEE_ARMOR, 4, STAT_COURAGE, 8, STAT_FEAR_RESIST_PERCENT, 50),
    RECORD(HAUBERK_UNIQUE, 7, STAT_MELEE_ARMOR, 16, STAT_MISSILE_ARMOR, 14, STAT_MAX_SPIRIT, 20),
    RECORD2(HELM_1, 1, STAT_MELEE_ARMOR, 2, STAT_MAX_SPIRIT, 3),
    RECORD2(HELM_2, 3, STAT_MELEE_ARMOR, 3, STAT_MAX_SPIRIT, 5),
    RECORD2(HELM_3, 5, STAT_MELEE_ARMOR, 4, STAT_MAX_SPIRIT, 8),
    RECORD2(HELM_4, 7, STAT_MELEE_ARMOR, 5, STAT_MAX_SPIRIT, 11),
    RECORD2(HELM_5, 9, STAT_MELEE_ARMOR, 6, STAT_MAX_SPIRIT, 15),
    RECORD2(MAIL_1, 1, STAT_MELEE_ARMOR, 2, STAT_MAX_HP, 5),
    RECORD2(MAIL_2, 3, STAT_MELEE_ARMOR, 5, STAT_MAX_HP, 8),
    RECORD2(MAIL_3, 5, STAT_MELEE_ARMOR, 8, STAT_MAX_HP, 12),
    RECORD2(MAIL_4, 7, STAT_MELEE_ARMOR, 11, STAT_MAX_HP, 16),
    RECORD2(MAIL_5, 9, STAT_MELEE_ARMOR, 14, STAT_MAX_HP, 20),
    RECORD2(BARROW_CROWN_UNIQUE, 7, STAT_MELEE_ARMOR, 5, STAT_MAX_SPIRIT, 20),
    RECORD2(MASK_UNIQUE, 8, STAT_MELEE_ARMOR, 6, STAT_COURAGE, 6),
};
const ItemGfx WraithArmorGfx[] = {
    [ARMOR_STOCK] = ITEM_GFX(ArmorHood, LOOT_TYPE_HELM_HOOD),
    ITEM_GFX(ArmorCoif, LOOT_TYPE_HELM_HOOD),
    ITEM_GFX(ArmorPothelm, LOOT_TYPE_HELM_METAL),
    ITEM_GFX(ArmorGreathelm, LOOT_TYPE_HELM_METAL),
    ITEM_GFX(ArmorCrown, LOOT_TYPE_HELM_CROWN),
    ITEM_GFX(ArmorBoiledLeather, LOOT_TYPE_ARMOR_TIER_1),
    ITEM_GFX(ArmorRingmail, LOOT_TYPE_ARMOR_TIER_2),
    ITEM_GFX(ArmorScaleMail, LOOT_TYPE_ARMOR_TIER_2),
    ITEM_GFX(ArmorLamellar, LOOT_TYPE_ARMOR_TIER_3),
    ITEM_GFX(ArmorFootmanPlate, LOOT_TYPE_ARMOR_TIER_3),
    ITEM_GFX(UniqueMageCrownOfUlrick, LOOT_TYPE_HELM_CROWN),
    ITEM_GFX(UniqueFomacSMail, LOOT_TYPE_ARMOR_TIER_3),
    ITEM_GFX(ArmorCap, LOOT_TYPE_HELM_HOOD),
    ITEM_GFX(ArmorHelm, LOOT_TYPE_HELM_METAL),
    ITEM_GFX(ArmorPothelm, LOOT_TYPE_HELM_METAL),
    ITEM_GFX(ArmorImperialHelm, LOOT_TYPE_HELM_METAL),
    ITEM_GFX(ArmorGreathelm, LOOT_TYPE_HELM_METAL),
    ITEM_GFX(ArmorChainmail, LOOT_TYPE_ARMOR_TIER_2),
    ITEM_GFX(ArmorBrigandine, LOOT_TYPE_ARMOR_TIER_2),
    ITEM_GFX(ArmorDoubleMail, LOOT_TYPE_ARMOR_TIER_3),
    ITEM_GFX(ArmorBreastplate, LOOT_TYPE_ARMOR_TIER_3),
    ITEM_GFX(ArmorLorica, LOOT_TYPE_ARMOR_TIER_3),
    ITEM_GFX(UniqueShiningHelmOfJaeSunbae, LOOT_TYPE_HELM_CROWN),
    ITEM_GFX(UniqueErlinaSGuamanianBoarHelm, LOOT_TYPE_HELM_METAL),
};
const ItemMetadata WraithArmorItemFlags[] = {
    [ARMOR_STOCK] = META5(ITEM_FLAG_SLOT_HELMET, 0x20),
    META5(ITEM_FLAG_SLOT_ARMOR, 0x20),
    META(ITEM_FLAG_SLOT_HELMET | WRAITH_UNIQUE_FLAGS, 0x20),
    META(ITEM_FLAG_SLOT_ARMOR | WRAITH_UNIQUE_FLAGS, 0x20),
    META5(ITEM_FLAG_SLOT_HELMET, 0x20),
    META5(ITEM_FLAG_SLOT_ARMOR, 0x20),
    META(ITEM_FLAG_SLOT_HELMET | WRAITH_UNIQUE_FLAGS, 0x20),
    META(ITEM_FLAG_SLOT_HELMET | WRAITH_UNIQUE_FLAGS, 0x20),
};

const ItemStatRecord WraithAccessoryItems[] = {
    [RING_STOCK] = RECORD1(RING_1, 1, STAT_COURAGE, 1),
    RECORD1(RING_2, 3, STAT_COURAGE, 2),
    RECORD2(RING_3, 5, STAT_COURAGE, 3, STAT_MAX_SPIRIT, 5),
    RECORD2(RING_4, 7, STAT_COURAGE, 4, STAT_MAX_SPIRIT, 10),
    RECORD2(RING_5, 9, STAT_COURAGE, 5, STAT_MAX_SPIRIT, 15),
    RECORD2(RING_UNIQUE, 7, STAT_COURAGE, 6, STAT_CRIT_CHANCE_PERCENT, 9),
    RECORD1(PENDANT_1, 1, STAT_MAX_SPIRIT, 5),
    RECORD1(PENDANT_2, 3, STAT_MAX_SPIRIT, 8),
    RECORD2(PENDANT_3, 5, STAT_MAX_SPIRIT, 10, STAT_COURAGE, 2),
    RECORD2(PENDANT_4, 7, STAT_MAX_SPIRIT, 15, STAT_COURAGE, 3),
    RECORD2(PENDANT_5, 9, STAT_MAX_SPIRIT, 20, STAT_COURAGE, 4),
    RECORD2(SIGNET_UNIQUE, 7, STAT_COURAGE, 5, STAT_MAX_SPIRIT, 15),
    RECORD2(HORN_UNIQUE, 7, STAT_COURAGE, 6, STAT_MAX_SPIRIT, 10),
    RECORD2(KHAMUL_UNIQUE, 8, STAT_COURAGE, 5, STAT_MAX_HP, 20),
    RECORD2(VOICE_UNIQUE, 8, STAT_COURAGE, 8, STAT_MAX_SPIRIT, 20),
};
const ItemGfx WraithAccessoryGfx[] = {
    [RING_STOCK] = ITEM_GFX(AccessoryClasp, LOOT_TYPE_ACCESSORY_TRINKET),
    ITEM_GFX(AccessoryCharm, LOOT_TYPE_ACCESSORY_TRINKET),
    ITEM_GFX(AccessoryBrooch, LOOT_TYPE_ACCESSORY_TRINKET),
    ITEM_GFX(AccessoryMedallion, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryTalisman, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryFiringa, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryPendant, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryChoker, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryNecklace, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryAmulet, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryMedallion, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryFiringa, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryCollar, LOOT_TYPE_ACCESSORY_AMULET),
    ITEM_GFX(AccessoryKeepsake, LOOT_TYPE_ACCESSORY_TRINKET),
    ITEM_GFX(UniqueTheUnknownAmuletOfKempor, LOOT_TYPE_ACCESSORY_AMULET),
};
const ItemMetadata WraithAccessoryItemFlags[] = {
    [RING_STOCK] = META5(ITEM_FLAG_SLOT_JEWELRY, 0x25),
    META(ITEM_FLAG_SLOT_JEWELRY | WRAITH_UNIQUE_FLAGS, 0x25),
    META5(ITEM_FLAG_SLOT_JEWELRY, 0x25),
    META(ITEM_FLAG_SLOT_JEWELRY | WRAITH_UNIQUE_FLAGS, 0x25),
    META(ITEM_FLAG_SLOT_JEWELRY | WRAITH_UNIQUE_FLAGS, 0x25),
    META(ITEM_FLAG_SLOT_JEWELRY | WRAITH_UNIQUE_FLAGS, 0x25),
    META(ITEM_FLAG_SLOT_JEWELRY | WRAITH_UNIQUE_FLAGS, 0x25),
};

const ItemStatRecord WraithCloakItems[] = {
    [SHROUD_STOCK] = RECORD2(SHROUD_1, 1, STAT_DODGE_PERCENT, 2, STAT_MISSILE_ARMOR, 1),
    RECORD2(SHROUD_2, 3, STAT_DODGE_PERCENT, 4, STAT_MISSILE_ARMOR, 2),
    RECORD2(SHROUD_3, 5, STAT_DODGE_PERCENT, 6, STAT_MISSILE_ARMOR, 3),
    RECORD2(SHROUD_4, 7, STAT_DODGE_PERCENT, 8, STAT_MISSILE_ARMOR, 4),
    RECORD2(SHROUD_5, 9, STAT_DODGE_PERCENT, 10, STAT_MISSILE_ARMOR, 6),
    RECORD2(SHROUD_UNIQUE, 7, STAT_DODGE_PERCENT, 12, STAT_MISSILE_ARMOR, 6),
    RECORD2(WRAP_1, 1, STAT_DODGE_PERCENT, 3, STAT_SPEED_PERCENT, 1),
    RECORD2(WRAP_2, 3, STAT_DODGE_PERCENT, 5, STAT_SPEED_PERCENT, 2),
    RECORD2(WRAP_3, 5, STAT_DODGE_PERCENT, 7, STAT_SPEED_PERCENT, 3),
    RECORD2(WRAP_4, 7, STAT_DODGE_PERCENT, 9, STAT_SPEED_PERCENT, 4),
    RECORD2(WRAP_5, 9, STAT_DODGE_PERCENT, 12, STAT_SPEED_PERCENT, 5),
    RECORD2(MARSHES_UNIQUE, 7, STAT_DODGE_PERCENT, 10, STAT_MAX_SPIRIT, 15),
};
const ItemGfx WraithCloakGfx[] = {
    [SHROUD_STOCK] = ITEM_GFX(CloakCape, LOOT_TYPE_CLOAK_TIER_1),
    ITEM_GFX(CloakMantle, LOOT_TYPE_CLOAK_TIER_1),
    ITEM_GFX(CloakCloak, LOOT_TYPE_CLOAK_TIER_1),
    ITEM_GFX(CloakHeavyCloak, LOOT_TYPE_CLOAK_TIER_2),
    ITEM_GFX(CloakRobe, LOOT_TYPE_CLOAK_TIER_3),
    ITEM_GFX(UniqueAramatSGarnetCowl, LOOT_TYPE_CLOAK_TIER_2),
    ITEM_GFX(CloakCoat, LOOT_TYPE_CLOAK_TIER_1),
    ITEM_GFX(CloakTabard, LOOT_TYPE_CLOAK_TIER_2),
    ITEM_GFX(CloakVestments, LOOT_TYPE_CLOAK_TIER_2),
    ITEM_GFX(CloakWinterCloak, LOOT_TYPE_CLOAK_TIER_3),
    ITEM_GFX(CloakRobe, LOOT_TYPE_CLOAK_TIER_3),
    ITEM_GFX(UniqueLoveBlessedCloakOfAdinAri, LOOT_TYPE_CLOAK_TIER_3),
};
const ItemMetadata WraithCloakItemFlags[] = {
    [SHROUD_STOCK] = META5(ITEM_FLAG_SLOT_CLOAK, 0x22),
    META(ITEM_FLAG_SLOT_CLOAK | WRAITH_UNIQUE_FLAGS, 0x22),
    META5(ITEM_FLAG_SLOT_CLOAK, 0x22),
    META(ITEM_FLAG_SLOT_CLOAK | WRAITH_UNIQUE_FLAGS, 0x22),
};

const ItemStatRecord WraithGloveItems[] = {
    [GAUNTLETS_STOCK] = RECORD2(GAUNTLETS_1, 1, STAT_STRENGTH, 1, STAT_CRIT_CHANCE_PERCENT, 2),
    RECORD2(GAUNTLETS_2, 3, STAT_STRENGTH, 2, STAT_CRIT_CHANCE_PERCENT, 3),
    RECORD2(GAUNTLETS_3, 5, STAT_STRENGTH, 3, STAT_CRIT_CHANCE_PERCENT, 4),
    RECORD2(GAUNTLETS_4, 7, STAT_STRENGTH, 4, STAT_CRIT_CHANCE_PERCENT, 5),
    RECORD2(GAUNTLETS_5, 9, STAT_STRENGTH, 5, STAT_CRIT_CHANCE_PERCENT, 6),
    RECORD2(GAUNTLETS_UNIQUE, 8, STAT_STRENGTH, 6, STAT_CRIT_CHANCE_PERCENT, 9),
    RECORD2(GRIPS_1, 1, STAT_ACCURACY, 1, STAT_CRIT_CHANCE_PERCENT, 3),
    RECORD2(GRIPS_2, 3, STAT_ACCURACY, 2, STAT_CRIT_CHANCE_PERCENT, 4),
    RECORD2(GRIPS_3, 5, STAT_ACCURACY, 3, STAT_CRIT_CHANCE_PERCENT, 5),
    RECORD2(GRIPS_4, 7, STAT_ACCURACY, 4, STAT_CRIT_CHANCE_PERCENT, 6),
    RECORD2(GRIPS_5, 9, STAT_ACCURACY, 5, STAT_CRIT_CHANCE_PERCENT, 8),
    RECORD2(UNLIT_UNIQUE, 7, STAT_ACCURACY, 6, STAT_CRIT_CHANCE_PERCENT, 10),
};
const ItemGfx WraithGloveGfx[] = {
    [GAUNTLETS_STOCK] = ITEM_GFX(GloveLeatherGloves, LOOT_TYPE_GLOVE),
    ITEM_GFX(GloveStuddedGloves, LOOT_TYPE_GLOVE),
    ITEM_GFX(GloveChainGloves, LOOT_TYPE_GLOVE),
    ITEM_GFX(GlovePlatedGloves, LOOT_TYPE_GLOVE),
    ITEM_GFX(GloveGauntlets, LOOT_TYPE_GLOVE),
    ITEM_GFX(UniqueGauntletsOfNumbers, LOOT_TYPE_GLOVE),
    ITEM_GFX(GloveClothGloves, LOOT_TYPE_GLOVE),
    ITEM_GFX(GloveHalfGloves, LOOT_TYPE_GLOVE),
    ITEM_GFX(GloveScaleGloves, LOOT_TYPE_GLOVE),
    ITEM_GFX(GloveChainGloves, LOOT_TYPE_GLOVE),
    ITEM_GFX(GloveGauntlets, LOOT_TYPE_GLOVE),
    ITEM_GFX(UniqueDustenBardicGloves, LOOT_TYPE_GLOVE),
};
const ItemMetadata WraithGloveItemFlags[] = {
    [GAUNTLETS_STOCK] = META5(ITEM_FLAG_SLOT_GLOVES, 0x24),
    META(ITEM_FLAG_SLOT_GLOVES | WRAITH_UNIQUE_FLAGS, 0x24),
    META5(ITEM_FLAG_SLOT_GLOVES, 0x24),
    META(ITEM_FLAG_SLOT_GLOVES | WRAITH_UNIQUE_FLAGS, 0x24),
};

const ItemStatRecord WraithShoeItems[] = {
    [GREAVES_STOCK] = RECORD2(GREAVES_1, 1, STAT_SPEED_PERCENT, 2, STAT_DEFENSE, 1),
    RECORD2(GREAVES_2, 3, STAT_SPEED_PERCENT, 4, STAT_DEFENSE, 1),
    RECORD2(GREAVES_3, 5, STAT_SPEED_PERCENT, 5, STAT_DEFENSE, 2),
    RECORD2(GREAVES_4, 7, STAT_SPEED_PERCENT, 6, STAT_DEFENSE, 2),
    RECORD2(GREAVES_5, 9, STAT_SPEED_PERCENT, 8, STAT_DEFENSE, 3),
    RECORD2(GREAVES_UNIQUE, 7, STAT_SPEED_PERCENT, 10, STAT_DEFENSE, 4),
    RECORD2(TREADS_1, 1, STAT_SPEED_PERCENT, 3, STAT_COURAGE, 1),
    RECORD2(TREADS_2, 3, STAT_SPEED_PERCENT, 4, STAT_COURAGE, 1),
    RECORD2(TREADS_3, 5, STAT_SPEED_PERCENT, 6, STAT_COURAGE, 2),
    RECORD2(TREADS_4, 7, STAT_SPEED_PERCENT, 7, STAT_COURAGE, 2),
    RECORD2(TREADS_5, 9, STAT_SPEED_PERCENT, 9, STAT_COURAGE, 3),
    RECORD2(SADDLE_UNIQUE, 7, STAT_SPEED_PERCENT, 12, STAT_COURAGE, 4),
};
const ItemGfx WraithShoeGfx[] = {
    [GREAVES_STOCK] = ITEM_GFX(ShoeWrappings, LOOT_TYPE_SHOE_TIER_1),
    ITEM_GFX(ShoeShoes, LOOT_TYPE_SHOE_TIER_1),
    ITEM_GFX(ShoeBoots, LOOT_TYPE_SHOE_TIER_2),
    ITEM_GFX(ShoeRidingBoots, LOOT_TYPE_SHOE_TIER_2),
    ITEM_GFX(ShoeSollerets, LOOT_TYPE_SHOE_TIER_2),
    ITEM_GFX(ShoeSollerets, LOOT_TYPE_SHOE_TIER_2),
    ITEM_GFX(ShoeSandals, LOOT_TYPE_SHOE_TIER_1),
    ITEM_GFX(ShoeClogs, LOOT_TYPE_SHOE_TIER_1),
    ITEM_GFX(ShoeBoots, LOOT_TYPE_SHOE_TIER_2),
    ITEM_GFX(ShoeRidingBoots, LOOT_TYPE_SHOE_TIER_2),
    ITEM_GFX(ShoeSollerets, LOOT_TYPE_SHOE_TIER_2),
    ITEM_GFX(UniqueNickohareSFoot, LOOT_TYPE_SHOE_TIER_2),
};
const ItemMetadata WraithShoeItemFlags[] = {
    [GREAVES_STOCK] = META5(ITEM_FLAG_SLOT_BOOTS, 0x21),
    META(ITEM_FLAG_SLOT_BOOTS | WRAITH_UNIQUE_FLAGS, 0x21),
    META5(ITEM_FLAG_SLOT_BOOTS, 0x21),
    META(ITEM_FLAG_SLOT_BOOTS | WRAITH_UNIQUE_FLAGS, 0x21),
};

// Wraith prefixes, appended to every prefix table a wraith slot uses. Items save their prefix
// index, so new prefixes go at the end regardless of tier.
#define WRAITH_PREFIX_COUNT 20
#define WRAITH_PREFIX_RECORDS                                                                      \
    RECORD0(PREFIX_DREADFUL, 1), RECORD1(PREFIX_COLD, 1, STAT_MELEE_ARMOR, 3),                     \
        RECORD0(PREFIX_HUNGERING, 2), RECORD1(PREFIX_SHADOWED, 2, STAT_DODGE_PERCENT, 4),          \
        RECORD1(PREFIX_WITHERING, 3, STAT_DAMAGE_SLASH, 4),                                        \
        RECORD1(PREFIX_GRAVE_COLD, 3, STAT_FEAR_RESIST_PERCENT, 10), RECORD0(PREFIX_TERRIBLE, 4),  \
        RECORD0(PREFIX_COMMANDING, 4), RECORD0(PREFIX_RAVENOUS, 5),                                \
        RECORD1(PREFIX_SORCEROUS, 5, STAT_MAX_SPIRIT, 10), RECORD0(PREFIX_PETRIFYING, 6),          \
        RECORD1(PREFIX_DEATHLESS, 6, STAT_MAX_HP, 15), RECORD0(PREFIX_SOUL_EATING, 7),             \
        RECORD1(PREFIX_UNDYING, 8, STAT_MAX_HP, 25), RECORD0(PREFIX_ABYSSAL, 8),                   \
        RECORD1(PREFIX_NAMELESS, 9, STAT_DAMAGE_SLASH, 8), RECORD0(PREFIX_GRAVEBOUND, 3),        \
        RECORD0(PREFIX_LURKING, 2), RECORD0(PREFIX_HARROWING, 4), RECORD0(PREFIX_NECROMANTIC, 6)

#define WEAPON_PREFIX_STOCK ARRAY_COUNT(WeaponPrefixes)
#define ARMOR_PREFIX_STOCK ARRAY_COUNT(ArmorPrefixes)
#define ACCESSORY_PREFIX_STOCK ARRAY_COUNT(AccessoryPrefixes)
#define CLOAK_PREFIX_STOCK ARRAY_COUNT(CloakPrefixes)
#define GLOVE_SHOE_PREFIX_STOCK ARRAY_COUNT(GloveShoePrefixes)

const ItemStatRecord WraithWeaponPrefixes[] = {
    [WEAPON_PREFIX_STOCK] = WRAITH_PREFIX_RECORDS,
};
const AffixFlags WraithWeaponPrefixFlags[WEAPON_PREFIX_STOCK + WRAITH_PREFIX_COUNT] = {{0}};
const ItemStatRecord WraithArmorPrefixes[] = {
    [ARMOR_PREFIX_STOCK] = WRAITH_PREFIX_RECORDS,
};
const AffixFlags WraithArmorPrefixFlags[ARMOR_PREFIX_STOCK + WRAITH_PREFIX_COUNT] = {{0}};
const ItemStatRecord WraithAccessoryPrefixes[] = {
    [ACCESSORY_PREFIX_STOCK] = WRAITH_PREFIX_RECORDS,
};
const AffixFlags WraithAccessoryPrefixFlags[ACCESSORY_PREFIX_STOCK + WRAITH_PREFIX_COUNT] = {{0}};
const ItemStatRecord WraithCloakPrefixes[] = {
    [CLOAK_PREFIX_STOCK] = WRAITH_PREFIX_RECORDS,
};
const AffixFlags WraithCloakPrefixFlags[CLOAK_PREFIX_STOCK + WRAITH_PREFIX_COUNT] = {{0}};
const ItemStatRecord WraithGloveShoePrefixes[] = {
    [GLOVE_SHOE_PREFIX_STOCK] = WRAITH_PREFIX_RECORDS,
};
const AffixFlags WraithGloveShoePrefixFlags[GLOVE_SHOE_PREFIX_STOCK + WRAITH_PREFIX_COUNT] = {{0}};

// Wraith suffixes. New suffixes go at the end regardless of tier.
#define WRAITH_SUFFIX_COUNT 16
const ItemStatRecord WraithItemSuffixes[] = {
    [WRAITH_SUFFIX_STOCK_COUNT] = RECORD1(SUFFIX_NINE, 1, STAT_CRIT_CHANCE_PERCENT, 5),
    RECORD1(SUFFIX_BARROWS, 1, STAT_MELEE_ARMOR, 4),
    RECORD1(SUFFIX_SHADOW, 2, STAT_DODGE_PERCENT, 5),
    RECORD1(SUFFIX_COLD_IRON, 2, STAT_DAMAGE_SLASH, 3),
    RECORD0(SUFFIX_HUNGER, 3),
    RECORD0(SUFFIX_THRALLDOM, 3),
    RECORD0(SUFFIX_ANGMAR, 4),
    RECORD0(SUFFIX_DESPAIR, 4),
    RECORD0(SUFFIX_DOL_GULDUR, 5),
    RECORD1(SUFFIX_MINAS_MORGUL, 5, STAT_MAX_SPIRIT, 15),
    RECORD0(SUFFIX_WITCH_KING, 6),
    RECORD1(SUFFIX_DARK_LORD, 7, STAT_DAMAGE_SLASH, 6),
    RECORD0(SUFFIX_GRAVE, 2),
    RECORD0(SUFFIX_UNSEEN, 3),
    RECORD0(SUFFIX_TERROR, 5),
    RECORD0(SUFFIX_HOST, 4),
};
const AffixFlags WraithItemSuffixFlags[WRAITH_SUFFIX_STOCK_COUNT + WRAITH_SUFFIX_COUNT] = {{0}};

// Two WraithGearEffect values carried by a wraith base, prefix or suffix.
typedef struct WraithEffects
{
    // WraithGearEffect
    u8 effect0;
    s8 value0;
    // WraithGearEffect
    u8 effect1;
    s8 value1;
    // WraithUnique, or WRAITH_NO_UNIQUE
    u8 unique;
} WraithEffects;

#define FX1(e, v) {.effect0 = WRAITH_FX_##e, .value0 = (v), .unique = WRAITH_NO_UNIQUE}
#define FX2(e0, v0, e1, v1)                                                                        \
    {.effect0 = WRAITH_FX_##e0, .value0 = (v0), .effect1 = WRAITH_FX_##e1, .value1 = (v1),         \
     .unique = WRAITH_NO_UNIQUE}
#define UNIQUE(u) {.unique = WRAITH_UNIQUE_##u}

static const WraithEffects MaceEffects[] = {
    FX1(DREAD_PER_HIT, 2), FX1(DREAD_PER_HIT, 3), FX1(DREAD_PER_HIT, 4),
    FX1(DREAD_PER_HIT, 5), FX1(DREAD_PER_HIT, 6), UNIQUE(WITCH_KINGS_MACE),
    FX2(TERROR_SPIRIT, 1, DREAD_PER_HIT, 1), FX2(TERROR_SPIRIT, 2, DREAD_PER_HIT, 2),
    FX2(TERROR_SPIRIT, 2, DREAD_PER_HIT, 3), FX2(TERROR_SPIRIT, 3, DREAD_PER_HIT, 3),
    FX2(TERROR_SPIRIT, 4, DREAD_PER_HIT, 4), UNIQUE(BANNER_OF_ANGMAR),
};
static const WraithEffects BladeEffects[] = {
    FX1(LIFE_DRAIN, 1), FX1(LIFE_DRAIN, 1), FX1(LIFE_DRAIN, 2),
    FX1(LIFE_DRAIN, 2), FX1(LIFE_DRAIN, 3), UNIQUE(MORGUL_KNIFE),
    FX1(UNSEEN_DAMAGE, 10), FX1(UNSEEN_DAMAGE, 15), FX1(UNSEEN_DAMAGE, 20),
    FX1(UNSEEN_DAMAGE, 25), FX1(UNSEEN_DAMAGE, 35), UNIQUE(BLACK_DART),
};
static const WraithEffects ArmorEffects[] = {
    FX1(FEAR_REACH, 4), FX1(FEAR_REACH, 6), FX1(FEAR_REACH, 8),  FX1(FEAR_REACH, 10),
    FX1(FEAR_REACH, 12), FX1(WARD, 2),      FX1(WARD, 3),        FX1(WARD, 4),
    FX1(WARD, 5),        FX1(WARD, 6),      UNIQUE(IRON_CROWN),  UNIQUE(BARROW_HAUBERK),
    FX1(THRALL_DAMAGE, 1), FX1(THRALL_DAMAGE, 2), FX1(THRALL_DAMAGE, 3), FX1(THRALL_DAMAGE, 4),
    FX1(THRALL_DAMAGE, 5), FX2(THRALL_FRAMES, 30, WARD, 1), FX2(THRALL_FRAMES, 45, WARD, 1),
    FX2(THRALL_FRAMES, 60, WARD, 2), FX2(THRALL_FRAMES, 90, WARD, 2),
    FX2(THRALL_FRAMES, 120, WARD, 3), UNIQUE(BARROW_CROWN), UNIQUE(DOL_GULDUR_MASK),
};
static const WraithEffects RingEffects[] = {
    FX1(KILL_SPIRIT, 1), FX1(KILL_SPIRIT, 1), FX1(KILL_SPIRIT, 2),
    FX1(KILL_SPIRIT, 2), FX1(KILL_SPIRIT, 3), UNIQUE(NINTH_RING),
    FX2(TERROR_SPIRIT, 1, FEAR_REACH, 3), FX2(TERROR_SPIRIT, 1, FEAR_REACH, 4),
    FX2(TERROR_SPIRIT, 2, FEAR_REACH, 6), FX2(TERROR_SPIRIT, 2, FEAR_REACH, 8),
    FX2(TERROR_SPIRIT, 3, FEAR_REACH, 10), UNIQUE(CARN_DUM_SIGNET), UNIQUE(MORGUL_HORN),
    UNIQUE(RING_OF_KHAMUL), UNIQUE(VOICE_OF_SAURON),
};
static const WraithEffects ShroudEffects[] = {
    FX1(UNSEEN, 15), FX1(UNSEEN, 20), FX1(UNSEEN, 25),
    FX1(UNSEEN, 30), FX1(UNSEEN, 40), UNIQUE(SHROUD_OF_UNBEING),
    FX2(UNSEEN, 10, UNSEEN_DAMAGE, 5), FX2(UNSEEN, 15, UNSEEN_DAMAGE, 8),
    FX2(UNSEEN, 20, UNSEEN_DAMAGE, 10), FX2(UNSEEN, 25, UNSEEN_DAMAGE, 12),
    FX2(UNSEEN, 30, UNSEEN_DAMAGE, 15), UNIQUE(DEAD_MARSHES),
};
static const WraithEffects GauntletEffects[] = {
    FX1(FRIGHT_DAMAGE, 4),  FX1(FRIGHT_DAMAGE, 6),  FX1(FRIGHT_DAMAGE, 8),
    FX1(FRIGHT_DAMAGE, 10), FX1(FRIGHT_DAMAGE, 12), UNIQUE(BLACK_CAPTAIN),
    FX2(UNSEEN_DAMAGE, 5, LIFE_DRAIN, 1), FX2(UNSEEN_DAMAGE, 8, LIFE_DRAIN, 1),
    FX2(UNSEEN_DAMAGE, 12, LIFE_DRAIN, 1), FX2(UNSEEN_DAMAGE, 15, LIFE_DRAIN, 2),
    FX2(UNSEEN_DAMAGE, 20, LIFE_DRAIN, 2), UNIQUE(UNLIT_HAND),
};
static const WraithEffects GreavesEffects[] = {
    FX1(DRAIN_HEAL, 5),  FX1(DRAIN_HEAL, 8),  FX1(DRAIN_HEAL, 11),
    FX1(DRAIN_HEAL, 14), FX1(DRAIN_HEAL, 18), UNIQUE(MORGUL_VALE),
    FX2(TERROR_STUN, 10, FEAR_REACH, 2), FX2(TERROR_STUN, 15, FEAR_REACH, 3),
    FX2(TERROR_STUN, 20, FEAR_REACH, 4), FX2(TERROR_STUN, 25, FEAR_REACH, 5),
    FX2(TERROR_STUN, 30, FEAR_REACH, 6), UNIQUE(FELL_BEAST_SADDLE),
};

static const WraithEffects PrefixEffects[] = {
    FX1(DREAD_PER_HIT, 4),
    FX1(WARD, 3),
    FX1(LIFE_DRAIN, 2),
    FX1(UNSEEN, 20),
    FX1(FRIGHT_DAMAGE, 8),
    FX1(WARD, 5),
    FX1(DREAD_PER_HIT, 8),
    FX1(THRALL_DAMAGE, 3),
    FX1(LIFE_DRAIN, 4),
    FX1(KILL_SPIRIT, 2),
    FX1(TERROR_STUN, 30),
    FX1(DRAIN_HEAL, 15),
    FX2(KILL_SPIRIT, 3, LIFE_DRAIN, 3),
    FX1(WARD, 8),
    FX2(FEAR_REACH, 12, DREAD_PER_HIT, 6),
    FX1(FRIGHT_DAMAGE, 15),
    FX1(THRALL_FRAMES, 60),
    FX1(UNSEEN_DAMAGE, 15),
    FX1(TERROR_SPIRIT, 3),
    FX2(THRALL_DAMAGE, 3, THRALL_FRAMES, 45),
};

static const WraithEffects SuffixEffects[] = {
    FX1(DREAD_PER_HIT, 3),
    FX1(WARD, 3),
    FX1(UNSEEN, 30),
    FX1(TERROR_STUN, 15),
    FX1(LIFE_DRAIN, 3),
    FX1(THRALL_DAMAGE, 4),
    FX1(FEAR_REACH, 10),
    FX1(FRIGHT_DAMAGE, 12),
    FX1(DRAIN_HEAL, 20),
    FX1(KILL_SPIRIT, 2),
    FX2(DREAD_PER_HIT, 6, TERROR_STUN, 20),
    FX2(FEAR_REACH, 8, FRIGHT_DAMAGE, 8),
    FX1(THRALL_FRAMES, 45),
    FX1(UNSEEN_DAMAGE, 20),
    FX1(TERROR_SPIRIT, 4),
    FX2(THRALL_DAMAGE, 3, THRALL_FRAMES, 30),
};

// A missing or extra row in any parallel table fails the build here.
_Static_assert(ARRAY_COUNT(WraithMaceItems) == MACE_STOCK + MACE_COUNT, "WraithMaceItems row count");
_Static_assert(ARRAY_COUNT(WraithMaceGfx) == MACE_STOCK + MACE_COUNT, "WraithMaceGfx row count");
_Static_assert(ARRAY_COUNT(WraithMaceItemFlags) == MACE_STOCK + MACE_COUNT, "WraithMaceItemFlags row count");
_Static_assert(ARRAY_COUNT(WraithSword1HItems) == BLADE_STOCK + BLADE_COUNT, "WraithSword1HItems row count");
_Static_assert(ARRAY_COUNT(WraithSword1HGfx) == BLADE_STOCK + BLADE_COUNT, "WraithSword1HGfx row count");
_Static_assert(ARRAY_COUNT(WraithSword1HItemFlags) == BLADE_STOCK + BLADE_COUNT, "WraithSword1HItemFlags row count");
_Static_assert(ARRAY_COUNT(WraithArmorItems) == ARMOR_STOCK + ARMOR_COUNT, "WraithArmorItems row count");
_Static_assert(ARRAY_COUNT(WraithArmorGfx) == ARMOR_STOCK + ARMOR_COUNT, "WraithArmorGfx row count");
_Static_assert(ARRAY_COUNT(WraithArmorItemFlags) == ARMOR_STOCK + ARMOR_COUNT, "WraithArmorItemFlags row count");
_Static_assert(ARRAY_COUNT(WraithAccessoryItems) == RING_STOCK + RING_COUNT, "WraithAccessoryItems row count");
_Static_assert(ARRAY_COUNT(WraithAccessoryGfx) == RING_STOCK + RING_COUNT, "WraithAccessoryGfx row count");
_Static_assert(ARRAY_COUNT(WraithAccessoryItemFlags) == RING_STOCK + RING_COUNT, "WraithAccessoryItemFlags row count");
_Static_assert(ARRAY_COUNT(WraithCloakItems) == SHROUD_STOCK + SHROUD_COUNT, "WraithCloakItems row count");
_Static_assert(ARRAY_COUNT(WraithCloakGfx) == SHROUD_STOCK + SHROUD_COUNT, "WraithCloakGfx row count");
_Static_assert(ARRAY_COUNT(WraithCloakItemFlags) == SHROUD_STOCK + SHROUD_COUNT, "WraithCloakItemFlags row count");
_Static_assert(ARRAY_COUNT(WraithGloveItems) == GAUNTLETS_STOCK + GAUNTLETS_COUNT, "WraithGloveItems row count");
_Static_assert(ARRAY_COUNT(WraithGloveGfx) == GAUNTLETS_STOCK + GAUNTLETS_COUNT, "WraithGloveGfx row count");
_Static_assert(ARRAY_COUNT(WraithGloveItemFlags) == GAUNTLETS_STOCK + GAUNTLETS_COUNT, "WraithGloveItemFlags row count");
_Static_assert(ARRAY_COUNT(WraithShoeItems) == GREAVES_STOCK + GREAVES_COUNT, "WraithShoeItems row count");
_Static_assert(ARRAY_COUNT(WraithShoeGfx) == GREAVES_STOCK + GREAVES_COUNT, "WraithShoeGfx row count");
_Static_assert(ARRAY_COUNT(WraithShoeItemFlags) == GREAVES_STOCK + GREAVES_COUNT, "WraithShoeItemFlags row count");
_Static_assert(ARRAY_COUNT(WraithWeaponPrefixes) == WEAPON_PREFIX_STOCK + WRAITH_PREFIX_COUNT, "WraithWeaponPrefixes row count");
_Static_assert(ARRAY_COUNT(WraithArmorPrefixes) == ARMOR_PREFIX_STOCK + WRAITH_PREFIX_COUNT, "WraithArmorPrefixes row count");
_Static_assert(ARRAY_COUNT(WraithAccessoryPrefixes) == ACCESSORY_PREFIX_STOCK + WRAITH_PREFIX_COUNT, "WraithAccessoryPrefixes row count");
_Static_assert(ARRAY_COUNT(WraithCloakPrefixes) == CLOAK_PREFIX_STOCK + WRAITH_PREFIX_COUNT, "WraithCloakPrefixes row count");
_Static_assert(ARRAY_COUNT(WraithGloveShoePrefixes) == GLOVE_SHOE_PREFIX_STOCK + WRAITH_PREFIX_COUNT, "WraithGloveShoePrefixes row count");
_Static_assert(ARRAY_COUNT(WraithItemSuffixes) == WRAITH_SUFFIX_STOCK_COUNT + WRAITH_SUFFIX_COUNT, "WraithItemSuffixes row count");
_Static_assert(ARRAY_COUNT(MaceEffects) == MACE_COUNT, "MaceEffects row count");
_Static_assert(ARRAY_COUNT(BladeEffects) == BLADE_COUNT, "BladeEffects row count");
_Static_assert(ARRAY_COUNT(ArmorEffects) == ARMOR_COUNT, "ArmorEffects row count");
_Static_assert(ARRAY_COUNT(RingEffects) == RING_COUNT, "RingEffects row count");
_Static_assert(ARRAY_COUNT(ShroudEffects) == SHROUD_COUNT, "ShroudEffects row count");
_Static_assert(ARRAY_COUNT(GauntletEffects) == GAUNTLETS_COUNT, "GauntletEffects row count");
_Static_assert(ARRAY_COUNT(GreavesEffects) == GREAVES_COUNT, "GreavesEffects row count");
_Static_assert(ARRAY_COUNT(PrefixEffects) == WRAITH_PREFIX_COUNT, "PrefixEffects row count");
_Static_assert(ARRAY_COUNT(SuffixEffects) == WRAITH_SUFFIX_COUNT, "SuffixEffects row count");
_Static_assert(WRAITH_UNIQUE_COUNT <= 32, "uniques must fit the u32 mask");

// One item type that carries wraith bases.
typedef struct WraithItemType
{
    // ItemType
    u8 itemType;
    // stock base items before the wraith bases
    u8 stockBaseCount;
    // stock prefixes before the wraith prefixes
    u8 stockPrefixCount;
    // wraith base count
    u8 baseCount;
    // per wraith base effects, indexed by baseIndex - stockBaseCount
    const WraithEffects *bases;
} WraithItemType;

static const WraithItemType WraithItemTypes[] = {
    {ITEM_TYPE_MACE, MACE_STOCK, WEAPON_PREFIX_STOCK, MACE_COUNT, MaceEffects},
    {ITEM_TYPE_SWORD_1H, BLADE_STOCK, WEAPON_PREFIX_STOCK, BLADE_COUNT, BladeEffects},
    {ITEM_TYPE_ARMOR, ARMOR_STOCK, ARMOR_PREFIX_STOCK, ARMOR_COUNT, ArmorEffects},
    {ITEM_TYPE_ACCESSORY, RING_STOCK, ACCESSORY_PREFIX_STOCK, RING_COUNT, RingEffects},
    {ITEM_TYPE_CLOAK, SHROUD_STOCK, CLOAK_PREFIX_STOCK, SHROUD_COUNT, ShroudEffects},
    {ITEM_TYPE_GLOVES, GAUNTLETS_STOCK, GLOVE_SHOE_PREFIX_STOCK, GAUNTLETS_COUNT, GauntletEffects},
    {ITEM_TYPE_SHOES, GREAVES_STOCK, GLOVE_SHOE_PREFIX_STOCK, GREAVES_COUNT, GreavesEffects},
};

#define WRAITH_SLOT_UNIQUE_MAX 4

// One wraith drop slot: a run of five tiered bases plus the uniques it can roll instead.
typedef struct WraithDropSlot
{
    // drop weight, and all slots sum to 100
    u8 weight;
    // index into WraithItemTypes
    u8 typeIndex;
    // first tiered base, relative to the type's wraith bases
    u8 firstBase;
    // number of entries used in uniqueBases, 1..WRAITH_SLOT_UNIQUE_MAX
    u8 uniqueCount;
    // unique bases, relative to the type's wraith bases
    u8 uniqueBases[WRAITH_SLOT_UNIQUE_MAX];
} WraithDropSlot;

static const WraithDropSlot WraithDropSlots[] = {
    {7, 0, 0, 1, {5}},          // Mace
    {7, 0, 6, 1, {11}},         // Maul
    {5, 1, 0, 1, {5}},          // Blade
    {5, 1, 6, 1, {11}},         // Fang
    {7, 2, 0, 1, {10}},         // Crown
    {7, 2, 5, 1, {11}},         // Hauberk
    {7, 2, 12, 2, {22, 23}},    // Helm
    {7, 2, 17, 0, {0}},         // Mail
    {6, 3, 0, 3, {5, 11, 13}},  // Ring
    {6, 3, 6, 2, {12, 14}},     // Pendant
    {6, 4, 0, 1, {5}},          // Shroud
    {6, 4, 6, 1, {11}},         // Wrap
    {6, 5, 0, 1, {5}},          // Gauntlets
    {6, 5, 6, 1, {11}},         // Grips
    {6, 6, 0, 1, {5}},          // Greaves
    {6, 6, 6, 1, {11}},         // Treads
};

static const char *const EffectLabels[WRAITH_FX_COUNT] = {
    [WRAITH_FX_DREAD_PER_HIT] = " Dread per Hit",
    [WRAITH_FX_FEAR_REACH] = " Fear Reach",
    [WRAITH_FX_LIFE_DRAIN] = "% Life Drain",
    [WRAITH_FX_KILL_SPIRIT] = " Spirit per Kill",
    [WRAITH_FX_FRIGHT_DAMAGE] = "% Dmg vs Frightened",
    [WRAITH_FX_TERROR_STUN] = "s Terror",
    [WRAITH_FX_UNSEEN] = "s Unseen",
    [WRAITH_FX_DRAIN_HEAL] = "% Drain Healing",
    [WRAITH_FX_WARD] = "% Damage Ward",
    [WRAITH_FX_THRALL_DAMAGE] = " Thrall Damage",
    [WRAITH_FX_THRALL_FRAMES] = "s Thrall Time",
    [WRAITH_FX_UNSEEN_DAMAGE] = "% Unseen Damage",
    [WRAITH_FX_TERROR_SPIRIT] = " Spirit per Terror",
};

static const char *const UniqueDescriptions[WRAITH_UNIQUE_COUNT] = {
    [WRAITH_UNIQUE_WITCH_KINGS_MACE] = "Kills spread +40 Dread to nearby foes.",
    [WRAITH_UNIQUE_MORGUL_KNIFE] = "Melee hits embed Morgul shards. -25% melee damage.",
    [WRAITH_UNIQUE_IRON_CROWN] = "Double fear reach. Dread never fades. +15% damage taken.",
    [WRAITH_UNIQUE_BARROW_HAUBERK] = "Damage drains Spirit first, 1 Spirit per 2 damage.",
    [WRAITH_UNIQUE_NINTH_RING] = "Every ninth melee hit terrifies.",
    [WRAITH_UNIQUE_SHROUD_OF_UNBEING] = "Hits keep you Unseen. Unseen lasts half as long and its "
                                        "blows deal 5x.",
    [WRAITH_UNIQUE_BLACK_CAPTAIN] = "Double damage and +10% Life Drain against foes at full "
                                    "Dread.",
    [WRAITH_UNIQUE_MORGUL_VALE] = "Every 2s the nearest foe gains +25 Dread.",
    [WRAITH_UNIQUE_BANNER_OF_ANGMAR] = "One more thrall may serve you. Thralls deal +3 damage.",
    [WRAITH_UNIQUE_BARROW_CROWN] = "Foes near your thralls gain +15 Dread each pulse.",
    [WRAITH_UNIQUE_CARN_DUM_SIGNET] = "Command of the Nine spares one thrall.",
    [WRAITH_UNIQUE_BLACK_DART] = "Unseen blows plant a Morgul shard.",
    [WRAITH_UNIQUE_DEAD_MARSHES] = "The Unseen World lasts 50% longer.",
    [WRAITH_UNIQUE_UNLIT_HAND] = "A kill from Shadowstep's strike refunds its Spirit.",
    [WRAITH_UNIQUE_MORGUL_HORN] = "Cry of the Nazgul deals double damage and refunds half its "
                                  "Spirit.",
    [WRAITH_UNIQUE_DOL_GULDUR_MASK] = "Terrified foes burst on death for 8 damage and +50 Dread.",
    [WRAITH_UNIQUE_VOICE_OF_SAURON] = "Your aura is half as wide but spreads double Dread.",
    [WRAITH_UNIQUE_RING_OF_KHAMUL] = "Every kill heals 3% of your health.",
    [WRAITH_UNIQUE_FELL_BEAST_SADDLE] = "Fell Beast sweeps twice as wide and terrifies.",
};

static WraithGearStats gearStats[2];
// Equipped item words that gearStats was last built from.
static u32 equippedWords[2][INVENTORY_SLOT_BACKPACK_0];

asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global wraithGear_original_formatName\n"
    ".thumb_func\n"
    "wraithGear_original_formatName:\n"
    "    push {r4-r7, lr}\n"
    "    mov  r7, r10\n"
    "    mov  r6, r9\n"
    "    mov  r5, r8\n"
    "    ldr  r3, =item_affix_formatName + 8\n"
    "    bx   r3\n"
    "    .pool\n"
    ".balign 4\n"
    ".global wraithGear_original_getEquipSlot\n"
    ".thumb_func\n"
    "wraithGear_original_getEquipSlot:\n"
    "    push {r4-r7, lr}\n"
    "    mov  r7, r10\n"
    "    mov  r6, r9\n"
    "    mov  r5, r8\n"
    "    ldr  r3, =item_getEquipSlot + 8\n"
    "    bx   r3\n"
    "    .pool\n");

char *wraithGear_original_formatName(char *dst, Item item, u16 maxWidth);
u32 wraithGear_original_getEquipSlot(u32 itemHandle, u8 playerIndex);

static const WraithItemType *findType(u32 itemType)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(WraithItemTypes); i++)
    {
        if (WraithItemTypes[i].itemType == itemType)
        {
            return &WraithItemTypes[i];
        }
    }
    return NULL;
}

static bool32 isWraithPrefix(const WraithItemType *type, u32 prefixIndex)
{
    return prefixIndex != ITEM_AFFIX_NONE && prefixIndex >= type->stockPrefixCount;
}

static bool32 isWraithSuffix(u32 suffixIndex)
{
    return suffixIndex != ITEM_AFFIX_NONE && suffixIndex >= WRAITH_SUFFIX_STOCK_COUNT;
}

bool32 wraithGear_isWraithItem(Item item)
{
    const WraithItemType *type = findType(ITEM_TYPE(item));

    return type != NULL &&
           (item.d.baseIndex >= type->stockBaseCount || isWraithPrefix(type, item.d.prefixIndex) ||
            isWraithSuffix(item.d.suffixIndex));
}

static void addEffects(WraithGearStats *stats, const WraithEffects *fx)
{
    stats->effects[fx->effect0] += fx->value0;
    stats->effects[fx->effect1] += fx->value1;
    if (fx->unique != WRAITH_NO_UNIQUE)
    {
        stats->uniques |= 1u << fx->unique;
    }
}

static void addItem(WraithGearStats *stats, Item item)
{
    const WraithItemType *type = findType(ITEM_TYPE(item));
    u32 base;

    if (type == NULL)
    {
        return;
    }
    base = item.d.baseIndex - type->stockBaseCount;
    if (item.d.baseIndex >= type->stockBaseCount && base < type->baseCount)
    {
        addEffects(stats, &type->bases[base]);
    }
    if (isWraithPrefix(type, item.d.prefixIndex) &&
        item.d.prefixIndex - type->stockPrefixCount < WRAITH_PREFIX_COUNT)
    {
        addEffects(stats, &PrefixEffects[item.d.prefixIndex - type->stockPrefixCount]);
    }
    if (isWraithSuffix(item.d.suffixIndex) &&
        item.d.suffixIndex - WRAITH_SUFFIX_STOCK_COUNT < WRAITH_SUFFIX_COUNT)
    {
        addEffects(stats, &SuffixEffects[item.d.suffixIndex - WRAITH_SUFFIX_STOCK_COUNT]);
    }
}

void wraithGear_refresh(u8 playerIndex)
{
    WraithGearStats *stats = &gearStats[playerIndex];
    u32 slot;
    bool32 changed = FALSE;

    for (slot = 0; slot < INVENTORY_SLOT_BACKPACK_0; slot++)
    {
        changed |= equippedWords[playerIndex][slot] != PLAYER(playerIndex).inventory.array[slot].word;
        equippedWords[playerIndex][slot] = PLAYER(playerIndex).inventory.array[slot].word;
    }
    if (!changed)
    {
        return;
    }
    memory_memset(stats, 0, sizeof(*stats));
    for (slot = 0; slot < INVENTORY_SLOT_BACKPACK_0; slot++)
    {
        addItem(stats, PLAYER(playerIndex).inventory.array[slot]);
    }
    stats->effects[WRAITH_FX_NONE] = 0;
}

const WraithGearStats *wraithGear_stats(u8 playerIndex)
{
    return &gearStats[playerIndex];
}

const char *wraithGear_findString(s32 textId)
{
    if (textId < WRAITH_TEXT_ID_BASE || textId >= WRAITH_TEXT_ID_BASE + WRAITH_TEXT_COUNT)
    {
        return NULL;
    }
    return WraithStrings[textId - WRAITH_TEXT_ID_BASE];
}

/** Pick one of the first count entries, leaning toward the later (higher tier) ones. */
static u32 rollTiered(u32 count)
{
    s32 a = rng_rollRange(0, count - 1);
    s32 b = rng_rollRange(0, count - 1);

    return a > b ? a : b;
}

/** Number of leading records whose tier the current loot tier allows, at least minimum. */
static u32 countAllowed(const ItemStatRecord *records, u32 count, s32 maxTier, u32 minimum)
{
    u32 n = 0;

    while (n < count && records[n].tier <= maxTier)
    {
        n++;
    }
    return n < minimum ? minimum : n;
}

/** A random record whose tier is allowed. The first minimum records are always allowed. */
static u32 pickAllowed(const ItemStatRecord *records, u32 count, s32 maxTier, u32 minimum)
{
    u32 allowed = 0;
    u32 i;
    s32 k;

    for (i = 0; i < count; i++)
    {
        allowed += i < minimum || records[i].tier <= maxTier;
    }
    k = rng_rollRange(0, allowed - 1);
    for (i = 0; i < count; i++)
    {
        if ((i < minimum || records[i].tier <= maxTier) && k-- == 0)
        {
            return i;
        }
    }
    return 0;
}

static u8 rollWraithItem(Item *itemOut, s16 extraTreasurePercent)
{
    const WraithDropSlot *slot = WraithDropSlots;
    const WraithItemType *type;
    s32 bonus = extraTreasurePercent / 10;
    s32 uniqueChance;
    s32 roll = rng_rollRange(1, 100);
    u32 base;
    u32 prefix = ITEM_AFFIX_NONE;
    u32 suffix = ITEM_AFFIX_NONE;

    while (roll > slot->weight && slot < &WraithDropSlots[ARRAY_COUNT(WraithDropSlots) - 1])
    {
        roll -= slot->weight;
        slot++;
    }
    type = &WraithItemTypes[slot->typeIndex];
    uniqueChance = slot->uniqueCount == 0 ? 0 : 3 + g_LootTier / 3;

    bonus = bonus < 0 ? 0 : bonus > 20 ? 20 : bonus;
    roll = rng_rollRange(1, 100);
    if (roll <= uniqueChance)
    {
        base = type->stockBaseCount +
               slot->uniqueBases[rng_rollRange(0, slot->uniqueCount - 1)];
    }
    else
    {
        base = type->stockBaseCount + slot->firstBase;
        base += rollTiered(countAllowed(&ItemTypeInfos[type->itemType].baseItems[base],
                                        WRAITH_TIER_COUNT, g_LootTier, 1));
        if (roll <= uniqueChance + 45 + bonus)
        {
            prefix = type->stockPrefixCount +
                     pickAllowed(&ItemTypeInfos[type->itemType].prefixes[type->stockPrefixCount],
                                 WRAITH_PREFIX_COUNT, g_LootTier + 1, 2);
        }
        if (roll <= uniqueChance + 10 + bonus / 2)
        {
            suffix = WRAITH_SUFFIX_STOCK_COUNT +
                     pickAllowed(&WraithItemSuffixes[WRAITH_SUFFIX_STOCK_COUNT],
                                 WRAITH_SUFFIX_COUNT, g_LootTier + 1, 2);
        }
    }
    itemOut->word = type->itemType | (RUNE_NONE << 4) | (base << 8) | (prefix << 16) |
                    (suffix << 24);
    return ITEM_TYPE_INFO(*itemOut).baseGfx[base].lootType;
}

/**
 * The Ringwraith finds only wraith gear. Other heroes roll stock items.
 *
 * REPLACES (whole function): loot_generateItem.
 */
u8 wraithGear_generateItem(Item *itemOut, u8 playerIndex, s16 extraTreasurePercent, u8 unitType)
{
    u32 heroId;
    s32 roll;
    u8 i;
    u8 category;

    heroId = PLAYER(playerIndex).heroId;
    if (heroId == HERO_ID_RINGWRAITH)
    {
        return rollWraithItem(itemOut, extraTreasurePercent);
    }
    roll = rng_rollRange(1, 100);
    i = 0;
    while (i <= ITEM_TYPE_UNIQUE && roll > LootHeroItemTypeRolls[heroId].thresholds[i])
    {
        i++;
    }
    if (unitType == LOOT_UNIT_TYPE_CONTAINER && i == ITEM_TYPE_UNIQUE)
    {
        i = ITEM_TYPE_ARMOR;
    }
    itemOut->d.itemType = (ItemType)i;

    category = loot_rollBaseItem(itemOut, playerIndex);
    if (itemOut->d.itemType != ITEM_TYPE_EMPTY)
    {
        loot_rollPrefix(itemOut, extraTreasurePercent, heroId, unitType);
        loot_rollSuffix(itemOut, extraTreasurePercent);
    }
    return category;
}

static u32 uniqueOf(Item item)
{
    const WraithItemType *type = findType(ITEM_TYPE(item));
    u32 base;

    if (type == NULL || item.d.baseIndex < type->stockBaseCount)
    {
        return WRAITH_NO_UNIQUE;
    }
    base = item.d.baseIndex - type->stockBaseCount;
    return base < type->baseCount ? type->bases[base].unique : WRAITH_NO_UNIQUE;
}

/** HUD text color for an orange name, or red when no BG palette bank is free for the ramp. */
static u8 orangeTextColor(void)
{
    s32 bank;

    for (bank = 15; bank > 0; bank--)
    {
        volatile u16 *colors = (volatile u16 *)(BG_PALETTE_RAM + bank * 32);
        bool32 free = TRUE;
        u32 i;

        for (i = 0; i < 16 && free; i++)
        {
            u16 expected = i == ORANGE_COLOR_INDEX       ? ORANGE_BRIGHT
                           : i == ORANGE_COLOR_INDEX + 1 ? ORANGE_MID
                           : i == ORANGE_COLOR_INDEX + 2 ? ORANGE_DARK
                                                         : 0;

            free = colors[i] == 0 || colors[i] == expected;
        }
        if (free)
        {
            colors[ORANGE_COLOR_INDEX] = ORANGE_BRIGHT;
            colors[ORANGE_COLOR_INDEX + 1] = ORANGE_MID;
            colors[ORANGE_COLOR_INDEX + 2] = ORANGE_DARK;
            return (bank << 4) | ORANGE_COLOR_INDEX;
        }
    }
    return HUD_TEXT_COLOR_MENU_RED;
}

/**
 * Wraith uniques are named in orange.
 *
 * REPLACES (entry hook): item_affix_formatName.
 */
char *wraithGear_formatName(char *dst, Item item, u16 maxWidth)
{
    wraithGear_original_formatName(dst, item, maxWidth);
    if (uniqueOf(item) != WRAITH_NO_UNIQUE)
    {
        g_PlayerHuds[ACTIVE_PLAYER_INDEX].textColor = orangeTextColor();
    }
    return dst;
}

/**
 * Only the Ringwraith can equip wraith gear.
 *
 * REPLACES (entry hook): item_getEquipSlot.
 */
u32 wraithGear_getEquipSlot(u32 itemHandle, u8 playerIndex)
{
    Item item;

    item.word = itemHandle;
    if (wraithGear_isWraithItem(item) && !ringwraith_isHero(playerIndex))
    {
        return INVENTORY_SLOT_BACKPACK_0;
    }
    return wraithGear_original_getEquipSlot(itemHandle, playerIndex);
}

static char *appendLiteral(char *dst, const char *src)
{
    char *limit = g_TextStringBuffer + TEXT_STRING_BUFFER_SIZE - 1;

    while (*src != '\0' && dst < limit)
    {
        *dst++ = *src++;
    }
    *dst = '\0';
    return dst;
}

static char *appendInt(char *dst, s32 value)
{
    text_formatInt(value, dst);
    while (*dst != '\0')
    {
        dst++;
    }
    return dst;
}

static char *appendEffect(char *dst, u32 effect, s32 value)
{
    dst = appendLiteral(dst, value < 0 ? "-" : "+");
    value = value < 0 ? -value : value;
    if (effect == WRAITH_FX_TERROR_STUN || effect == WRAITH_FX_UNSEEN ||
        effect == WRAITH_FX_THRALL_FRAMES)
    {
        // frames as seconds with one decimal
        dst = appendInt(dst, value / 60);
        dst = appendLiteral(dst, ".");
        dst = appendInt(dst, value % 60 / 6);
    }
    else
    {
        dst = appendInt(dst, value);
    }
    return appendLiteral(dst, EffectLabels[effect]);
}

/** Item info text of a wraith item: its stats, wraith effects and unique power. */
char *wraithGear_describe(Item item)
{
    char *dst = g_TextStringBuffer;
    char *end;
    WraithGearStats stats;
    u32 unique = uniqueOf(item);
    u32 effect;

    memory_memset(&stats, 0, sizeof(stats));
    addItem(&stats, item);
    memory_memset(dst, 0, TEXT_STRING_BUFFER_SIZE);
    end = item_affix_accumulateStats(dst, item);
    for (effect = WRAITH_FX_NONE + 1; effect < WRAITH_FX_COUNT; effect++)
    {
        if (stats.effects[effect] != 0)
        {
            if (end != dst)
            {
                end = appendLiteral(end, ", ");
            }
            end = appendEffect(end, effect, stats.effects[effect]);
        }
    }
    if (unique != WRAITH_NO_UNIQUE)
    {
        if (end != dst)
        {
            end = appendLiteral(end, ". ");
        }
        end = appendLiteral(end, UniqueDescriptions[unique]);
    }
    return dst;
}

#endif
