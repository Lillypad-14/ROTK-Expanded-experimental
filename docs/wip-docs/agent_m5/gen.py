T = '''// header declares this void; the ROM returns 0 (change prototype to u32)
#define {name} {name}_hdr
#define sprite_selectAnimation sprite_selectAnimation_hdr
#include "actor.h"
#include "player.h"
#include "sfx.h"
#include "skill.h"
#include "sprite.h"
#include "variables.h"
#undef {name}
#undef sprite_selectAnimation

// ROM passes pose and facing variant as s8
void sprite_selectAnimation(Actor *actor, void *animationBank, s8 pose, s8 facingVariant);
{decl}
/**
 * {doc}
 *
 * @romaddress {addr}
 */
u32 {name}(Actor *playerActor)
{{
    u32 playerIndex = playerActor->as.combat.playerIndex;
    s32 level = (s8)PLAYER(playerIndex).activeSkillLevels[{enum}];

    playerActor->as.combat.head.player.animationBank =
        (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId];
    playerActor->as.combat.requestedPose = PLAYER(playerIndex).castingActiveSkill + 1;
    sprite_selectAnimation(playerActor, (void *)HeroCastAnimationBanks[PLAYER(playerIndex).heroId],
                           (s8)(PLAYER(playerIndex).castingActiveSkill + 1),
                           (s8)FacingVariants[playerActor->facing]);
{after}    playerActor->actionState = {state};
    playerActor->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
{store}    playerActor->as.combat.actionStateFlags |= {fl};
{sfx}    return 0;
}}
'''
def write(**k):
    k.setdefault('decl',''); k.setdefault('after',''); k.setdefault('sfx','')
    open(k['name']+'.c','w').write(T.format(**k))
