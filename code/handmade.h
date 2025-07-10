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

#include "libs/stb_truetype.h"

#define WORDLE_LENGTH 5

typedef enum
{
    SquareColor_Gray = 0,
    SquareColor_Yellow = 1,
    SquareColor_Green = 2,
    SquareColor_Count
} square_colors;

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

struct game_state
{
    u32 PatternGrid[6][5];
    u32 SelectedColor;
    u32 ExportedPatternIndex;
    
    stbtt_fontinfo FontInfo;
    s32 FontAscent;
    s32 FontDescent;
    s32 FontLineGap;
    v2 FontBoundingBox[2];
};

#define HANDMADE_H
#endif
