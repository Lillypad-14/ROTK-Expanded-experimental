#pragma once

#include "types.h"

enum HackLibTimelineAnchor
{
    HACKLIB_TIMELINE_OWNER = 0,
    HACKLIB_TIMELINE_TARGET = 1,
    HACKLIB_TIMELINE_POSITION = 2,
};

enum HackLibTimelineEventKind
{
    HACKLIB_TIMELINE_VISUAL = 0,
    HACKLIB_TIMELINE_RELEASE = 1,
    HACKLIB_TIMELINE_FINISH = 2,
};

typedef struct HackLibTimelineEvent
{
    u16 tick;
    u16 effect;
    u8 anchor;
} HackLibTimelineEvent;

typedef struct HackLibCastTimeline
{
    const HackLibTimelineEvent *events;
    u16 releaseTick;
    u16 recoveryTicks;
    u8 count;
} HackLibCastTimeline;

typedef struct HackLibTimelineCursor
{
    const HackLibCastTimeline *definition;
    u16 elapsed;
    u8 next;
    bool released;
    bool active;
} HackLibTimelineCursor;

// Returning FALSE stops playback immediately, including remaining events at the same tick.
typedef bool (*HackLibTimelineEmit)(u8 kind, const HackLibTimelineEvent *event, void *context);

bool hackLib_startTimeline(HackLibTimelineCursor *cursor, const HackLibCastTimeline *definition);
bool hackLib_stepTimeline(HackLibTimelineCursor *cursor, HackLibTimelineEmit emit, void *context);
extern const HackLibCastTimeline HackLibCastTimelines[];
extern const char *const HackLibCastTimelineNames[];

#include "cast_timeline_ids.h"
