#include "lib/cast_timeline.h"
#include "lib/skill_fx.h"

#include "cast_timeline_data.inc"

bool hackLib_startTimeline(HackLibTimelineCursor *cursor, const HackLibCastTimeline *definition)
{
    *cursor = (HackLibTimelineCursor){0};
    if (definition == NULL || definition->count > 16 ||
        (definition->count && definition->events == NULL) ||
        (u32)definition->releaseTick + definition->recoveryTicks > 3600)
    {
        return FALSE;
    }
    for (u32 i = 0; i < definition->count; i++)
    {
        const HackLibTimelineEvent *event = &definition->events[i];
        if (event->tick > definition->releaseTick + definition->recoveryTicks ||
            (i && event->tick < definition->events[i - 1].tick) ||
            event->anchor > HACKLIB_TIMELINE_POSITION ||
            event->effect >= HACKLIB_VISUAL_EFFECT_COUNT)
        {
            return FALSE;
        }
    }
    cursor->definition = definition;
    cursor->active = TRUE;
    return TRUE;
}

bool hackLib_stepTimeline(HackLibTimelineCursor *cursor, HackLibTimelineEmit emit, void *context)
{
    if (!cursor->active || emit == NULL)
    {
        return FALSE;
    }
    const HackLibCastTimeline *definition = cursor->definition;
    if (!cursor->released && cursor->elapsed == definition->releaseTick)
    {
        cursor->released = TRUE;
        if (!emit(HACKLIB_TIMELINE_RELEASE, NULL, context))
        {
            cursor->active = FALSE;
            return FALSE;
        }
    }
    while (cursor->next < definition->count &&
           definition->events[cursor->next].tick <= cursor->elapsed)
    {
        const HackLibTimelineEvent *event = &definition->events[cursor->next++];
        if (!emit(HACKLIB_TIMELINE_VISUAL, event, context))
        {
            cursor->active = FALSE;
            return FALSE;
        }
    }
    if (cursor->elapsed == definition->releaseTick + definition->recoveryTicks)
    {
        cursor->active = FALSE;
        emit(HACKLIB_TIMELINE_FINISH, NULL, context);
    }
    else
    {
        cursor->elapsed++;
    }
    return cursor->active;
}
