#define mission_script_copyGroup hdr_dummy
#include "mission.h"
#undef mission_script_copyGroup

u16 unk_0800c7f8(u16 n);
u16 unk_0800c7d8(u16 n);
void unk_0800c728(const u8 *src, u8 *dst, u8 size);
asm(".global unk_0800c7f8\n.thumb_set unk_0800c7f8, 0x0800c7f8+1");
asm(".global unk_0800c7d8\n.thumb_set unk_0800c7d8, 0x0800c7d8+1");
asm(".global unk_0800c728\n.thumb_set unk_0800c728, 0x0800c728+1");

/**
 * Copy one spawn group's actor params into the group state.
 *
 * @return state-area cursor past the group
 *
 * @romaddress 0x0800c248
 */
u8 *mission_script_copyGroup(const u8 *groupSection, u8 *dest, u8 groupIndex)
{
    u32 idx = groupIndex * 2;
    const u8 *table = groupSection + 2;
    u16 step = *(u16 *)(table + idx);
    const u8 *src = groupSection + step;
    const u8 *group = src;
    u8 *base = dest;
    u8 count = *src;
    u8 i;

    *(u16 *)dest = count;
    step = unk_0800c7f8(count + 2);
    src += step;
    step = unk_0800c7d8(count + 1);
    for (i = 0; i < count; i++)
    {
        dest += step;
        *(u16 *)(base + 2 + i * 2) = dest - base;
        step = group[2 + i];
        unk_0800c728(src, dest, step);
        src += step;
        step += 8;
    }
    dest += step;
    return dest;
}
