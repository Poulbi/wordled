/* date = May 13th 2025 1:41 pm */

#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

#if COMPILER_GNU
#include <x86intrin.h>
#endif
//
// TODO(casey): Remove math.h
//
#include <math.h>

inline
s32 SignOf(s32 Value)
{
    s32 Result = (Value >= 0) ? 1 : -1;
    return Result;
}

inline 
r32 AbsoluteValue(r32 Real32)
{
    r32 Result = fabs(Real32);
    return Result;
}

inline 
s32 RoundReal32ToInt32(r32 Real32)
{
    s32 Result = (s32)(roundf(Real32));
    return Result;
}

inline 
u32 RoundReal32ToUInt32(r32 Real32)
{
    u32 Result = (u32)(roundf(Real32));
    return Result;
}

inline 
u32 TruncateReal32ToUInt32(r32 Real32)
{
    u32 Result = (u32)Real32;
    return Result;
}

inline 
s32 TruncateReal32ToInt32(r32 Real32)
{
    s32 Result = (s32)Real32;
    return Result;
}

inline 
s32 FloorReal32ToInt32(r32 Real32)
{
    s32 Result = (s32)floorf(Real32);
    return Result;
}

inline 
s32 CeilReal32ToInt32(r32 Real32)
{
    s32 Result = (s32)ceilf(Real32);
    return Result;
}
inline 
r32 Sin(r32 Angle)
{
    r32 Result = sinf(Angle);
    return Result;
}

inline 
r32 Cos(r32 Angle)
{
    r32 Result = cosf(Angle);
    return Result;
}

inline 
r32 Atan2(r32 Y, r32 X)
{
    r32 Result = atan2f(Y, X);
    return Result;
    
}
struct bit_scan_result
{
    b32 Found;
    u32 Index;
}
;
internal inline 
bit_scan_result FindLeastSignificantSetBit(u32 Value)
{
    bit_scan_result Result = {};
    
#if 0
#elif COMPILER_GNU
    Result.Index = __builtin_ffs(Value);
    if(Result.Index)
    {
        Result.Found = true;
        Result.Index--;
    }
#else
    for(u32 Test = 0;
        Test < 32;
        Test++)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif
    
    return Result;
}

u32 RotateLeft(u32 Value, u32 Count)
{
    u32 Result = 0;
    
#if 0
#elif COMPILER_GNU
    Result = _rotl(Value, Count);
#endif
    
    return Result;
}

u32 RotateRight(u32 Value, u32 Count)
{
    u32 Result = 0;
    
#if 0
#elif COMPILER_GNU
    Result = _rotr(Value, Count);
#endif
    
    return Result;
}


inline
r32 SquareRoot(r32 Real32)
{
    r32 Result = sqrtf(Real32);
    return Result;
}


#endif //HANDMADE_INTRINSICS_H
