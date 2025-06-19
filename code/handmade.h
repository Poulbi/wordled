#if !defined(HANDMADE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "handmade_platform.h"
#include "handmade_math.h"

struct memory_arena
{
    memory_index Size;
    u8 *Base;
    memory_index Used;
};

void
InitializeArena(memory_arena *Arena, memory_index Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (u8 *)Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, type) ((type *)PushSize((Arena), (sizeof(type))))
#define PushArray(Arena, Count, type) (type *)PushSize((Arena), (sizeof(type))*(Count)) 
void *
PushSize(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->Used + Size) < Arena->Size);
    
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    
    return Result;
}

#include "handmade_intrinsics.h"

struct loaded_bitmap
{
    int Width;
    int Height;
    u32 *Pixels;
};

struct color_rgb
{
    union
    {
        struct
        {
            r32 R;
            r32 G;
            r32 B;
        };
        r32 E[3];
    };
};

struct view
{
    u32 PointsToPixels;
    
    v2 SizePixels;
    v2 SizePoints;
    
    v2 TopLeft;
    v2 BottomRight;
    // NOTE(luca): These pixels are added to points to center them when rendering.
    v2 PointPad;
    
    v2 CenterOffset;
};

struct game_state
{
    r32 Slope;
    r32 Step;
    r32 B;
    r32 C;
};

#define HANDMADE_H
#endif
