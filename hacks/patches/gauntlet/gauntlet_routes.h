#pragma once

#include "types.h"

#define GAUNTLET_SECTOR_COUNT 3
#define GAUNTLET_ROUTE_MAP_COUNT 32

typedef struct GauntletRoutePoint
{
    s16 x;
    s16 y;
} GauntletRoutePoint;

typedef struct GauntletRoute
{
    const GauntletRoutePoint *points;
    u16 count;
} GauntletRoute;

// Connected floor routes and entry samples in the same map order as RoomMaps.
extern const GauntletRoute GauntletRoutes[GAUNTLET_ROUTE_MAP_COUNT][GAUNTLET_SECTOR_COUNT];

extern const u8 GauntletRouteEntrances[GAUNTLET_ROUTE_MAP_COUNT];
