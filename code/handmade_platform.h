/* date = May 12th 2025 10:53 am */

#ifndef HANDMADE_PLATFORM_H
#define HANDMADE_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif
    
    /*
      NOTE(casey):
    
      HANDMADE_INTERNAL:
        0 - Build for public release
        1 - Build for developer only
    
      HANDMADE_SLOW:
        0 - Not slow code allowed!
        1 - Slow code welcome.
    */
    
#include <stdint.h>
#include <stddef.h>
    
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif
    
#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif
    
#if !defined(COMPILER_GNU)
#define COMPILER_GNU 0
#endif
    
#if !COMPILER_MSVC && !COMPILER_LLVM && !COMPILER_GNU
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#elif __GNUC__
#undef COMPILER_GNU
#define COMPILER_GNU 1 
#else
    // TODO(casey): More compilerz!!!
#endif
#endif 
    
#define internal static 
#define local_persist static 
#define global_variable static
    
#define Pi32 3.14159265359f
    
#if HANDMADE_SLOW
    // TODO(casey): Complete assertion macro - don't worry everyone!
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif
    
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
    
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
    // TODO(casey): swap, min, max ... macros???
    
    
    typedef int8_t s8;
    typedef int16_t s16;
    typedef int32_t s32;
    typedef int64_t s64;
    typedef s32 b32;
    
    typedef uint8_t u8;
    typedef uint16_t u16;
    typedef uint32_t u32;
    typedef uint64_t u64;
    
    typedef size_t memory_index;
    
    typedef float r32;
    typedef double r64;
    
    
    typedef struct thread_context
    {
        int Placeholder;
    } thread_context;
    
    /*
      NOTE(casey): Services that the platform layer provides to the game
    */
#if HANDMADE_INTERNAL
    /* IMPORTANT(casey):
    
       These are NOT for doing anything in the shipping game - they are
       blocking and the write doesn't protect against lost data!
    */
    typedef struct debug_read_file_result
    {
        u32 ContentsSize;
        void *Contents;
    } debug_read_file_result;
    
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory, u64 MemorySize)
    typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
    
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *FileName)
    typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
    
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(thread_context *Thread, char *FileName, u32 MemorySize, void *Memory)
    typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
    
#endif
    
    /*
      NOTE(casey): Services that the game provides to the platform layer.
      (this may expand in the future - sound on separate thread, etc.)
    */
    
    // FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
    
    // TODO(casey): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
    typedef struct game_offscreen_buffer
    {
        // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
        void *Memory;
        int Width;
        int Height;
        int Pitch;
        int BytesPerPixel;
    } game_offscreen_buffer;
    
    typedef struct game_sound_output_buffer
    {
        int SamplesPerSecond;
        int SampleCount;
        s16 *Samples;
    } game_sound_output_buffer;
    
    typedef struct game_button_state
    {
        int HalfTransitionCount;
        b32 EndedDown;
    } game_button_state;
    
    typedef struct game_controller_input
    {
        b32 IsConnected;
        b32 IsAnalog;    
        r32 StickAverageX;
        r32 StickAverageY;
        
        union
        {
            game_button_state Buttons[12];
            struct
            {
                game_button_state MoveUp;
                game_button_state MoveDown;
                game_button_state MoveLeft;
                game_button_state MoveRight;
                
                game_button_state ActionUp;
                game_button_state ActionDown;
                game_button_state ActionLeft;
                game_button_state ActionRight;
                
                game_button_state LeftShoulder;
                game_button_state RightShoulder;
                
                game_button_state Back;
                game_button_state Start;
                
                // NOTE(casey): All buttons must be added above this line
                
                game_button_state Terminator;
            };
        };
    } game_controller_input;
    
    typedef struct game_input
    {
        game_button_state MouseButtons[5];
        s32 MouseX, MouseY, MouseZ;
        
        r32 dtForFrame;
        
        game_controller_input Controllers[5];
    } game_input;
    
    typedef struct game_memory
    {
        b32 IsInitialized;
        
        memory_index PermanentStorageSize;
        void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
        
        memory_index TransientStorageSize;
        void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
        
#if HANDMADE_INTERNAL
        debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
        debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
        debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
#endif
    } game_memory;
    
#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
    typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
    
    // NOTE(casey): At the moment, this has to be a very fast function, it cannot be
    // more than a millisecond or so.
    // TODO(casey): Reduce the pressure on this function's performance by measuring it
    // or asking about it, etc.
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer)
    typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
    
    inline u32
        SafeTruncateUInt64(u64 Value)
    {
        // TODO(casey): Defines for maximum values
        Assert(Value <= 0xFFFFFFFF);
        u32 Result = (u32)Value;
        return(Result);
    }
    
    inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
    {
        Assert(ControllerIndex < ArrayCount(Input->Controllers));
        
        game_controller_input *Result = &Input->Controllers[ControllerIndex];
        return(Result);
    }
    
#endif //HANDMADE_PLATFORM_H
    
#ifdef __cplusplus
}
#endif
