#define actor_gatePanel_writeTileBlock hdr_dummy
#include "actor.h"
#include "actor/gatePanel.h"
#undef actor_gatePanel_writeTileBlock

// Need names: 0x0803f338 (x, y, index, tileSet), 0x080346e4 (x, y, index).
void gatePanel_writeTile(u16 x, u16 y, u8 index, u8 tileSet);
void gatePanel_fixCollision(u16 x, u16 y, u8 index);

/**
 * Write the panel's cols x rows tile block at its map position, advancing the animation
 * cursor; also fixes up collision per tile.
 *
 * @romaddress 0x0801cebc
 */
void actor_gatePanel_writeTileBlock(Actor *actor)
{
    s32 x = ((s16 *)&actor->coords.x)[1];
    s32 y = ((s16 *)&actor->coords.y)[1];
    u8 index = actor->as.gatePanel.cursor;
    u8 row;
    u8 col;

    for (row = 0; row < actor->as.gatePanel.rows; row++)
    {
        for (col = 0; col < actor->as.gatePanel.cols; col++)
        {
            gatePanel_writeTile(x, y, index, actor->as.gatePanel.tileSet);
            gatePanel_fixCollision(x, y, index);
            x += 0x20;
            index++;
        }
        x = ((s16 *)&actor->coords.x)[1];
        y += 0x20;
    }
    actor->as.gatePanel.cursor = index;
}

void gatePanel_writeTile(u16 x, u16 y, u8 index, u8 tileSet)
{
}

void gatePanel_fixCollision(u16 x, u16 y, u8 index)
{
}
