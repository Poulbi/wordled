#if !defined(HANDMADE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "libs/linuxhmh/handmade_platform.h"
#include "handmade_math.h"

#undef STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

#define WORDLE_LENGTH 5

enum square_colors
{
    SquareColor_Gray,
    SquareColor_Yellow,
    SquareColor_Green,
    SquareColor_Count
};

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
#define color_rgb(A) color_rgb{(A), (A), (A)}

#define Max(A, B) (((A) > (B)) ? (A) : (B))
#define Min(A, B) (((A) < (B)) ? (A) : (B))
#define Clamp(A, B, C) (Max((A), Min((B), (C))))

inline color_rgb
operator*(color_rgb A, r32 B)
{
    color_rgb Result;
    
    Result.R = A.R * B;
    Result.G = A.G * B;
    Result.B = A.B * B;
    
    return Result;
}

inline color_rgb
operator*(r32 A, color_rgb B)
{
    color_rgb Result = B*A;
    
    return Result;
}

inline color_rgb
operator+(color_rgb A, r32 B)
{
    color_rgb Result;
    
    Result.R = A.R + B;
    Result.G = A.G + B;
    Result.B = A.B + B;
    
    return Result;
}

inline color_rgb
operator+(r32 A, color_rgb B)
{
    color_rgb Result = B+A;
    
    return Result;
}

inline color_rgb
operator-(color_rgb A)
{
    color_rgb Result;
    
    Result.R = -A.R;
    Result.G = -A.G;
    Result.B = -A.B;
    
    return Result;
}

struct game_font
{
    stbtt_fontinfo Info;
    s32 Ascent;
    s32 Descent;
    s32 LineGap;
    v2 BoundingBox[2];
    b32 Initialized; // For debugging.
};

struct game_state
{
    u32 PatternGrid[6][5];
    u32 SelectedColor;
    u32 ExportedPatternIndex;
    // TODO(luca): There is no need for utf8 since we know it will be between a-z.  We could use a single byte string and convert to char when appending to the input buffer.
    // But the only benefit would be storing less bytes which isn't really a big deal for 5 characters.
    rune WordleWord[WORDLE_LENGTH];
    
    game_font RegularFont;
    game_font ItalicFont;
    game_font BoldFont;
    
    b32 TextInputMode;
    rune TextInputText[5];
    u32 TextInputCount;
};

#define HANDMADE_H
#endif
