#include <stdio.h>
#include <time.h>
#include <x86intrin.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <signal.h>
#include <linux/limits.h>
#include <linux/input.h>
#include <alsa/asoundlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/extensions/Xrandr.h>

#include "handmade_platform.h"
#include "linux_handmade.h"

#define true 1
#define false 0

#define MAX_PLAYER_COUNT 4

#ifdef Assert
#undef Assert
#define Assert(Expression) \
if(!(Expression)) \
{ \
raise(SIGTRAP); \
}
#endif

// NOTE(luca): Bits are layed out over multiple bytes.  This macro checks which byte the bit will be set in.
#define IsEvdevBitSet(Bit, Array) (Array[(Bit) / 8] & (1 << ((Bit) % 8)))
#define BytesNeededForBits(Bits) ((Bits + 7) / 8)

global_variable b32 GlobalRunning;
global_variable b32 GlobalPaused;

void MemCpy(char *Dest, char *Source, size_t Count)
{
    while(Count--) *Dest++ = *Source++;
    
}

void MemSet(char *Dest, char Value, size_t Count)
{
    while(Count--) *Dest++ = Value;
}

int StrLen(char *String)
{
    size_t Result = 0;
    
    while(*String++) Result++;
    
    return Result;
}

void CatStrings(size_t SourceACount, char *SourceA,
                size_t SourceBCount, char *SourceB,
                size_t DestCount, char *Dest)
{
    for(size_t Index = 0;
        Index < SourceACount;
        Index++)
    {
        *Dest++ = *SourceA++;
    }
    
    for(size_t Index = 0;
        Index < SourceBCount;
        Index++)
    {
        *Dest++ = *SourceB++;
    }
}

struct linux_init_alsa_result
{
    snd_pcm_t *PCMHandle;
    snd_pcm_hw_params_t *PCMParams;
};

internal linux_init_alsa_result LinuxInitALSA()
{
    linux_init_alsa_result Result = {};
    
    int PCMResult = 0;
    if((PCMResult = snd_pcm_open(&Result.PCMHandle, "default",
                                 SND_PCM_STREAM_PLAYBACK, 0)) == 0) 
    {
        snd_pcm_hw_params_alloca(&Result.PCMParams);
        snd_pcm_hw_params_any(Result.PCMHandle, Result.PCMParams);
        u32 ChannelCount = 2;
        u32 SampleRate = 48000;
        
        if((PCMResult = snd_pcm_hw_params_set_access(Result.PCMHandle, Result.PCMParams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        {
            // TODO(luca): Logging
            // snd_strerror(pcm)
        }
        if((PCMResult = snd_pcm_hw_params_set_format(Result.PCMHandle, Result.PCMParams, SND_PCM_FORMAT_S16_LE)) < 0)
        {
            // TODO(luca): Logging
        }
        if((PCMResult = snd_pcm_hw_params_set_channels(Result.PCMHandle, Result.PCMParams, ChannelCount)) < 0)
        {
            // TODO(luca): Logging
        }
        if((PCMResult = snd_pcm_hw_params_set_rate_near(Result.PCMHandle, Result.PCMParams, &SampleRate, 0)) < 0) 
        {
            // TODO(luca): Logging
        }    
        if((PCMResult = snd_pcm_nonblock(Result.PCMHandle, 1)) < 0)
        {
            // TODO(luca): Logging
        }
        if((PCMResult = snd_pcm_reset(Result.PCMHandle)) < 0)
        {
            // TODO(luca): Logging
        }
        if((PCMResult = snd_pcm_hw_params(Result.PCMHandle, Result.PCMParams)) < 0)
        {
            // TODO(luca): Logging
        }
        
    }
    else
    {
        // TODO(luca): Logging
    }
    
    return Result;
}

internal void LinuxGetAxisInfo(linux_gamepad *GamePad, linux_gamepad_axes_enum Axis, int AbsAxis)
{
    input_absinfo AxesInfo = {};
    if(ioctl(GamePad->File, EVIOCGABS(AbsAxis), &AxesInfo) != -1)
    {
        GamePad->Axes[Axis].Minimum = AxesInfo.minimum;
        GamePad->Axes[Axis].Maximum = AxesInfo.maximum;
        GamePad->Axes[Axis].Fuzz = AxesInfo.fuzz;
        GamePad->Axes[Axis].Flat = AxesInfo.flat;
    }
}

internal void LinuxOpenGamePad(char *FilePath, linux_gamepad *GamePad)
{
    GamePad->File = open(FilePath, O_RDWR|O_NONBLOCK);
    
    if(GamePad->File != -1)
    {
        int Version = 0;
        int IsCompatible = true;
        
        // TODO(luca): Check versions
        ioctl(GamePad->File, EVIOCGVERSION, &Version);
        ioctl(GamePad->File, EVIOCGNAME(sizeof(GamePad->Name)), GamePad->Name);
        
        char SupportedEventBits[BytesNeededForBits(EV_MAX)] = {};
        if(ioctl(GamePad->File, EVIOCGBIT(0, sizeof(SupportedEventBits)), SupportedEventBits) != -1)
        {
            if(!IsEvdevBitSet(EV_ABS, SupportedEventBits))
            {
                // TODO(luca): Logging
                IsCompatible = false;
            }
            if(!IsEvdevBitSet(EV_KEY, SupportedEventBits))
            {
                // TODO(luca): Logging
                IsCompatible = false;
            }
        }
        
        char SupportedKeyBits[BytesNeededForBits(KEY_MAX)] = {};
        if(ioctl(GamePad->File, EVIOCGBIT(EV_KEY , sizeof(SupportedKeyBits)), SupportedKeyBits) != -1)
        {
            if(!IsEvdevBitSet(BTN_GAMEPAD, SupportedKeyBits))
            {
                // TODO(luca): Logging
                IsCompatible = false;
            }
        }
        
        GamePad->SupportsRumble = IsEvdevBitSet(EV_FF, SupportedEventBits);
        
        if(IsCompatible)
        {
            // NOTE(luca): Map evdev axes to my enum.
            LinuxGetAxisInfo(GamePad, LSTICKX, ABS_X);
            LinuxGetAxisInfo(GamePad, LSTICKY, ABS_Y);
            LinuxGetAxisInfo(GamePad, RSTICKX, ABS_RX);
            LinuxGetAxisInfo(GamePad, RSTICKY, ABS_RY);
            LinuxGetAxisInfo(GamePad, LSHOULDER, ABS_Z);
            LinuxGetAxisInfo(GamePad, RSHOULDER, ABS_RZ);
            LinuxGetAxisInfo(GamePad, DPADX, ABS_HAT0X);
            LinuxGetAxisInfo(GamePad, DPADY, ABS_HAT0Y);
            
            MemCpy(GamePad->FilePath, FilePath, StrLen(FilePath));
        }
        else
        {
            close(GamePad->File);
            *GamePad = {};
            GamePad->File = -1;
        }
    }
}

// TODO(luca): Make this work in the case of multiple displays.
internal r32 LinuxGetMonitorRefreshRate(Display *DisplayHandle, Window RootWindow)
{
    r32 Result = 0;
    
    void *LibraryHandle = dlopen("libXrandr.so.2", RTLD_NOW);
    
    if(LibraryHandle)
    {
        typedef XRRScreenResources *xrr_get_screen_resources(Display *Display, Window Window);
        typedef XRRCrtcInfo *xrr_get_crtc_info(Display* Display, XRRScreenResources *Resources, RRCrtc Crtc);
        
        xrr_get_screen_resources *XRRGetScreenResources = (xrr_get_screen_resources *)dlsym(LibraryHandle, "XRRGetScreenResources");
        xrr_get_crtc_info *XRRGetCrtcInfo = (xrr_get_crtc_info *)dlsym(LibraryHandle, "XRRGetCrtcInfo");
        
        XRRScreenResources *ScreenResources = XRRGetScreenResources(DisplayHandle, RootWindow);
        
        RRMode ActiveModeID = 0;
        for(int CRTCIndex = 0;
            CRTCIndex< ScreenResources->ncrtc;
            CRTCIndex++)
        {
            XRRCrtcInfo *CRTCInfo = XRRGetCrtcInfo(DisplayHandle, ScreenResources, ScreenResources->crtcs[CRTCIndex]);
            if(CRTCInfo->mode)
            {
                ActiveModeID = CRTCInfo->mode;
            }
        }
        
        r32 ActiveRate = 0;
        for(int ModeIndex = 0;
            ModeIndex < ScreenResources->nmode;
            ModeIndex++)
        {
            XRRModeInfo ModeInfo = ScreenResources->modes[ModeIndex];
            if(ModeInfo.id == ActiveModeID)
            {
                Assert(ActiveRate == 0);
                Result = (r32)ModeInfo.dotClock / ((r32)ModeInfo.hTotal * (r32)ModeInfo.vTotal);
            }
        }
        
        dlclose(LibraryHandle);
    }
    
    return Result;
}

internal void LinuxBuildFileNameFromExecutable(char *Dest, linux_state *State, char *FileName)
{
    char *Path = State->ExecutablePath;
    
    size_t LastSlash = 0;
    for(char *Scan = Path;
        *Scan;
        Scan++)
    {
        if(*Scan == '/')
        {
            LastSlash = Scan - Path;
        }
    }
    
    for(size_t Index = 0;
        Index < LastSlash + 1;
        Index++)
    {
        *Dest++ = *Path++;
    }
    
    while(*FileName)
        *Dest++ = *FileName++;
}

GAME_UPDATE_AND_RENDER(LinuxGameUpdateAndRenderStub) {}
GAME_GET_SOUND_SAMPLES(LinuxGameGetSoundSamplesStub) {}

internal linux_game_code LinuxLoadGameCode(char *LibraryPath)
{
    linux_game_code Result = {};
    
    Result.LibraryHandle = dlopen(LibraryPath, RTLD_NOW);
    if(Result.LibraryHandle)
    {
        Result.UpdateAndRender = (game_update_and_render *)dlsym(Result.LibraryHandle, "GameUpdateAndRender");
        Result.GetSoundSamples = (game_get_sound_samples *)dlsym(Result.LibraryHandle, "GameGetSoundSamples");
    }
    else
    {
        Result.UpdateAndRender = (game_update_and_render *)LinuxGameUpdateAndRenderStub;
        Result.GetSoundSamples = (game_get_sound_samples *)LinuxGameGetSoundSamplesStub;
    }
    
    return Result;
}

internal void LinuxUnloadGameCode(linux_game_code *Game)
{
    if(Game->LibraryHandle)
    {
        dlclose(Game->LibraryHandle);
    }
}

internal void LinuxProcessKeyPress(game_button_state *ButtonState, b32 IsDown)
{
    if(ButtonState->EndedDown != IsDown)
    {
        ButtonState->EndedDown = IsDown;
        ButtonState->HalfTransitionCount++;
    }
}

internal void LinuxBeginRecordingInput(linux_state *State, int SlotIndex)
{
    char Temp[64];
    sprintf(Temp, "loop_edit_%d.hmi", SlotIndex);
    char FileName[PATH_MAX] = {};
    LinuxBuildFileNameFromExecutable(FileName, State, Temp);
    
    int File = open(FileName, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0600); 
    if(State->InputRecordingFile != -1)
    {
        int BytesWritten = write(File, State->GameMemoryBlock, State->TotalSize);  
        if(BytesWritten != -1 &&
           BytesWritten == (int)State->TotalSize)
        {
            State->InputRecordingFile = File;
            State->InputRecordingIndex = SlotIndex;
        }
        else
        {
            // TODO(luca): Logging
        }
    }
    else
    {
        // TODO(luca): Logging
    }
    
}

internal void LinuxEndRecordingInput(linux_state *State, int SlotIndex)
{
    Assert(State->InputRecordingIndex);
    Assert(State->InputRecordingFile != -1);
    
    State->InputRecordingIndex = 0;
    
    if(close(State->InputRecordingFile) == -1)
    {
        State->InputRecordingFile = -1;
    }
    else
    {
        // TODO(luca): Logging
    }
    
}

internal void LinuxBeginInputPlayBack(linux_state *State, int SlotIndex)
{
    char Temp[64];
    sprintf(Temp, "loop_edit_%d.hmi", SlotIndex);
    char FileName[PATH_MAX] = {};
    LinuxBuildFileNameFromExecutable(FileName, State, Temp);
    
    int File = open(FileName, O_RDONLY, 0400);
    if(File != -1)
    {
        State->InputPlayingFile = File;
        
        int BytesRead = read(State->InputPlayingFile, State->GameMemoryBlock, State->TotalSize);
        if(BytesRead != -1)
        {
            State->InputPlayingIndex = SlotIndex;
        }
        else
        {
            // TODO(luca): Logging
        }
    }
    else
    {
        // TODO(luca): Logging
    }
    
}

internal void LinuxEndInputPlayBack(linux_state *State)
{
    Assert(State->InputPlayingIndex);
    Assert(State->InputPlayingFile != -1);
    
    if(close(State->InputPlayingFile) != -1)
    {
        State->InputPlayingFile = -1;
    }
    else
    {
        // TODO(luca): Logging
    }
    
    State->InputPlayingIndex = 0;
    
}

internal void LinuxRecordInput(linux_state *State, game_input *Input)
{
    int BytesWritten = write(State->InputRecordingFile, Input, sizeof(*Input));
    if(BytesWritten != -1 && 
       BytesWritten == (int)sizeof(*Input))
    {
    }
    else
    {
        // TODO: Logging
    }
    
}

internal void LinuxPlayBackInput(linux_state *State, game_input *Input)
{
    int BytesRead = read(State->InputPlayingFile, Input, sizeof(*Input));
    if(BytesRead != -1)
    {
        if(BytesRead == 0)
        {
            int PlayingIndex = State->InputPlayingIndex;
            LinuxEndInputPlayBack(State);
            LinuxBeginInputPlayBack(State, PlayingIndex);
            read(State->InputPlayingFile, Input, sizeof(*Input));
        }
    }
    
}

internal void LinuxHideCursor(Display *DisplayHandle, Window WindowHandle)
{
    XColor black = {};
    char NoData[8] = {};
    
    Pixmap BitmapNoData = XCreateBitmapFromData(DisplayHandle, WindowHandle, NoData, 8, 8);
    Cursor InvisibleCursor = XCreatePixmapCursor(DisplayHandle, BitmapNoData, BitmapNoData, 
                                                 &black, &black, 0, 0);
    XDefineCursor(DisplayHandle, WindowHandle, InvisibleCursor);
    XFreeCursor(DisplayHandle, InvisibleCursor);
    XFreePixmap(DisplayHandle, BitmapNoData);
}

internal void LinuxShowCursor(Display *DisplayHandle, Window WindowHandle)
{
    XUndefineCursor(DisplayHandle, WindowHandle);
}

internal void LinuxProcessPendingMessages(Display *DisplayHandle, Window WindowHandle,
                                          Atom WM_DELETE_WINDOW, linux_state *State, game_controller_input *KeyboardController)
{
    XEvent WindowEvent = {};
    while(XPending(DisplayHandle) > 0)
    {
        XNextEvent(DisplayHandle, &WindowEvent);
        switch (WindowEvent.type)
        {
            case KeyPress:
            case KeyRelease:
            {
                KeySym Symbol = XLookupKeysym(&WindowEvent.xkey, 0);
                b32 IsDown = (WindowEvent.type == KeyPress);
                
                if(0) {}
                else if(Symbol == XK_w)
                {
                    LinuxProcessKeyPress(&KeyboardController->MoveUp, IsDown);
                }
                else if(Symbol == XK_a)
                {
                    LinuxProcessKeyPress(&KeyboardController->MoveLeft, IsDown);
                }
                else if(Symbol == XK_r)
                {
                    LinuxProcessKeyPress(&KeyboardController->MoveDown, IsDown);
                }
                else if(Symbol == XK_s)
                {
                    LinuxProcessKeyPress(&KeyboardController->MoveRight, IsDown);
                }
                else if(Symbol == XK_Up)
                {
                    LinuxProcessKeyPress(&KeyboardController->ActionUp, IsDown);
                }
                else if(Symbol == XK_Left)
                {
                    LinuxProcessKeyPress(&KeyboardController->ActionLeft, IsDown);
                }
                else if(Symbol == XK_Down)
                {
                    LinuxProcessKeyPress(&KeyboardController->ActionDown, IsDown);
                }
                else if(Symbol == XK_Right)
                {
                    LinuxProcessKeyPress(&KeyboardController->ActionRight, IsDown);
                }
                else if(Symbol == XK_space)
                {
                    LinuxProcessKeyPress(&KeyboardController->Start, IsDown);
                }
                else if(Symbol == XK_p)
                {
                    if(IsDown)
                    {
                        GlobalPaused = !GlobalPaused;
                    }
                }
                else if(Symbol == XK_l)
                {
                    if(IsDown)
                    {
                        if(State->InputPlayingIndex == 0)
                        {
                            if(State->InputRecordingIndex == 0)
                            {
                                LinuxBeginRecordingInput(State, 1);
                            }
                            else
                            {
                                LinuxEndRecordingInput(State, 1);
                                LinuxBeginInputPlayBack(State, 1);
                            }
                        }
                        else
                        {
                            // TODO(luca): Reset buttons so they aren't held?
                            for(u32 ButtonIndex = 0;
                                ButtonIndex < ArrayCount(KeyboardController->Buttons);
                                ButtonIndex++)
                            {
                                KeyboardController->Buttons[ButtonIndex] = {};
                            }
                            LinuxEndInputPlayBack(State);
                        }
                    }
                }
                else if(Symbol == XK_Escape ||
                        Symbol == XK_q)
                {
                    GlobalRunning = false;
                }
                
            } break;
            case DestroyNotify:
            {
                XDestroyWindowEvent *Event = (XDestroyWindowEvent *)&WindowEvent;
                if(Event->window == WindowHandle)
                {
                    GlobalRunning = false;
                }
            } break;
            
            case ClientMessage:
            {
                XClientMessageEvent *Event = (XClientMessageEvent *)&WindowEvent;
                if((Atom)Event->data.l[0] == WM_DELETE_WINDOW)
                {
                    XDestroyWindow(DisplayHandle, WindowHandle);
                    GlobalRunning = false;
                }
            } break;
            
            case EnterNotify:
            {
                //LinuxHideCursor(DisplayHandle, WindowHandle);
            } break;
            
            case LeaveNotify:
            {
                //LinuxShowCursor(DisplayHandle, WindowHandle);
            } break;
            
        }
        
    }
    
}

internal void LinuxSetSizeHint(Display *DisplayHandle, Window WindowHandle,
                               int MinWidth, int MinHeight,
                               int MaxWidth, int MaxHeight)
{
    XSizeHints Hints = {};
    if(MinWidth > 0 && MinHeight > 0) Hints.flags |= PMinSize;
    if(MaxWidth > 0 && MaxHeight > 0) Hints.flags |= PMaxSize;
    
    Hints.min_width = MinWidth;
    Hints.min_height = MinHeight;
    Hints.max_width = MaxWidth;
    Hints.max_height = MaxHeight;
    
    XSetWMNormalHints(DisplayHandle, WindowHandle, &Hints);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};
    
    int File = open(FileName, O_RDONLY);
    if(File != -1)
    {
        struct stat FileStats = {};
        fstat(File, &FileStats);
        Result.ContentsSize = FileStats.st_size;
        Result.Contents = mmap(0, FileStats.st_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE, File, 0);
        
        close(File);
    }
    
    return Result;
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    munmap(Memory, MemorySize);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    b32 Result = false;
    
    int FD = open(FileName, O_CREAT|O_WRONLY|O_TRUNC, 00600);
    if(FD != -1)
    {
        if(write(FD, Memory, MemorySize) != MemorySize)
        {
            Result = true;
        }
        
        close(FD);
    }
    
    return Result;
}

internal struct timespec LinuxGetLastWriteTime(char *FilePath)
{
    struct timespec Result = {};
    
    struct stat LibraryFileStats = {};
    if(!stat(FilePath, &LibraryFileStats))
    {
        Result = LibraryFileStats.st_mtim;
    }
    
    return Result;
}

internal struct timespec LinuxGetWallClock()
{
    struct timespec Counter = {};
    clock_gettime(CLOCK_MONOTONIC, &Counter);
    return Counter;
}

internal s64 LinuxGetNSecondsElapsed(struct timespec Start, struct timespec End)
{
    s64 Result = 0;
    Result = ((s64)End.tv_sec*1000000000 + (s64)End.tv_nsec) - ((s64)Start.tv_sec*1000000000 + (s64)Start.tv_nsec);
    return Result;
}

internal r32 LinuxGetSecondsElapsed(struct timespec Start, struct timespec End)
{
    r32 Result = 0;
    Result = LinuxGetNSecondsElapsed(Start, End)/1000.0f/1000.0f/1000.0f;
    
    return Result;
}

internal r32 LinuxNormalizeAxisValue(s32 Value, linux_gamepad_axis Axis)
{
    r32 Result = 0;
    if(Value)
    {
        // ((value - min / max) - 0.5) * 2  
        r32 Normalized = ((r32)((r32)(Value - Axis.Minimum) / (r32)(Axis.Maximum - Axis.Minimum)) - 0.5f)*2;
        Result = Normalized;
    }
    Assert(Result <= 1.0f && Result >= -1.0f);
    
    return Result;
}

void LinuxDebugVerticalLine(game_offscreen_buffer *Buffer, int X, int Y, u32 Color)
{
    int Height = 32;
    
    if(X <= Buffer->Width && X >= 0 &&
       Y <= Buffer->Height - Height && Y <= 0)
    {
        u8 *Row = (u8 *)Buffer->Memory + Y*Buffer->Pitch + X*Buffer->BytesPerPixel;
        while(Height--)
        {
            *(u32 *)Row = Color;
            Row += Buffer->Pitch;
        }
    }
}

int main(int ArgC, char *Args[])
{
    Display *DisplayHandle = XOpenDisplay(0);
    
    if(DisplayHandle)
    {
        Window RootWindow = XDefaultRootWindow(DisplayHandle);
        int Screen = XDefaultScreen(DisplayHandle);
        int Width = 1920/2;
        int Height = 1080/2;
        int ScreenBitDepth = 24;
        XVisualInfo WindowVisualInfo = {};
        if(XMatchVisualInfo(DisplayHandle, Screen, ScreenBitDepth, TrueColor, &WindowVisualInfo))
        {
            XSetWindowAttributes WindowAttributes = {};
            WindowAttributes.bit_gravity = StaticGravity;
            WindowAttributes.background_pixel = 0xFF00FF;
            WindowAttributes.colormap = XCreateColormap(DisplayHandle, RootWindow, WindowVisualInfo.visual, AllocNone);
            WindowAttributes.event_mask = (StructureNotifyMask | 
                                           KeyPressMask | KeyReleaseMask |
                                           EnterWindowMask | LeaveWindowMask);
            u64 WindowAttributeMask = CWBitGravity | CWBackPixel | CWColormap | CWEventMask;
            
            Window WindowHandle = XCreateWindow(DisplayHandle, RootWindow,
                                                1920 - Width - 10, 10,
                                                Width, Height,
                                                0,
                                                WindowVisualInfo.depth, InputOutput,
                                                WindowVisualInfo.visual, WindowAttributeMask, &WindowAttributes);
            if(WindowHandle)
            {
                XStoreName(DisplayHandle, WindowHandle, "Handmade Window");
                LinuxSetSizeHint(DisplayHandle, WindowHandle, Width, Height, Width, Height);
                
                Atom WM_DELETE_WINDOW = XInternAtom(DisplayHandle, "WM_DELETE_WINDOW", False);
                if(!XSetWMProtocols(DisplayHandle, WindowHandle, &WM_DELETE_WINDOW, 1))
                {
                    // TODO(luca): Logging
                }
                
                XClassHint ClassHint = {};
                ClassHint.res_name = "Handmade Window";
                ClassHint.res_class = "Handmade Window";
                XSetClassHint(DisplayHandle, WindowHandle, &ClassHint);
                
                int BitsPerPixel = 32;
                int BytesPerPixel = BitsPerPixel/8;
                int WindowBufferSize = Width*Height*BytesPerPixel;
                char *WindowBuffer = (char *)malloc(WindowBufferSize);
                
                XImage *WindowImage = XCreateImage(DisplayHandle, WindowVisualInfo.visual, WindowVisualInfo.depth, ZPixmap, 0, WindowBuffer, Width, Height, BitsPerPixel, 0);
                GC DefaultGC = DefaultGC(DisplayHandle, Screen);
                
                
                linux_state LinuxState = {};
                MemCpy(LinuxState.ExecutablePath, Args[0], strlen(Args[0]));
                
                char LibraryFullPath[PATH_MAX] = {};
                char LibraryName[] = "handmade.so";
                LinuxBuildFileNameFromExecutable(LibraryFullPath, &LinuxState, LibraryName);
                linux_game_code Game = LinuxLoadGameCode(LibraryFullPath);
                Game.LibraryLastWriteTime = LinuxGetLastWriteTime(LibraryFullPath);
                
                game_memory GameMemory = {};
                
                GameMemory.PermanentStorageSize = Megabytes(64);
                GameMemory.TransientStorageSize = Gigabytes(1);
                GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
                GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
                GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
                
#if HANDMADE_INTERNAL
                void *BaseAddress = (void *)Terabytes(2);
#else
                void *BaseAddress = 0;
#endif
                
                // TODO(casey): TransientStorage needs to be broken into game transient and cache transient
                LinuxState.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
                LinuxState.GameMemoryBlock = mmap(BaseAddress, LinuxState.TotalSize, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, -1, 0);
                GameMemory.PermanentStorage = LinuxState.GameMemoryBlock;
                GameMemory.TransientStorage = (u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;
                
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];
                
                linux_gamepad GamePads[MAX_PLAYER_COUNT] = {};
                for(int GamePadIndex = 0;
                    GamePadIndex < MAX_PLAYER_COUNT;
                    GamePadIndex++)
                {
                    GamePads[GamePadIndex].File = -1;
                }
                
                char EventDirectoryName[] = "/dev/input/";
                DIR *EventDirectory = opendir(EventDirectoryName);
                struct dirent *Entry = 0;
                int GamePadIndex = 0;
                while((Entry = readdir(EventDirectory)))
                {
                    if(!strncmp(Entry->d_name, "event", sizeof("event") - 1))
                    {
                        char FilePath[PATH_MAX] = {};
                        CatStrings(sizeof(EventDirectoryName) - 1, EventDirectoryName,
                                   StrLen(Entry->d_name), Entry->d_name,
                                   sizeof(FilePath) - 1, FilePath);
                        if(GamePadIndex < MAX_PLAYER_COUNT)
                        {
                            linux_gamepad *GamePadAt = &GamePads[GamePadIndex];
                            LinuxOpenGamePad(FilePath, GamePadAt);
                            if(GamePadAt->File != -1)
                            {
#if 0
                                for(int AxesIndex = 0;
                                    AxesIndex < AXES_COUNT;
                                    AxesIndex++)
                                {
                                    linux_gamepad_axis Axis = GamePadAt->Axes[AxesIndex];
                                    printf("min: %d, max: %d, fuzz: %d, flat: %d\n", Axis.Minimum, Axis.Maximum, Axis.Fuzz, Axis.Flat);
                                }
#endif
                                GamePadIndex++;
                            }
                        }
                        else
                        {
                            // TODO(luca): Logging
                        }
                    }
                }
                
                game_offscreen_buffer OffscreenBuffer = {};
                OffscreenBuffer.Memory = WindowBuffer;
                OffscreenBuffer.Width = Width;
                OffscreenBuffer.Height = Height;
                OffscreenBuffer.BytesPerPixel = BytesPerPixel;
                OffscreenBuffer.Pitch = Width*BytesPerPixel;
                
                int LastFramesWritten = 0;
                unsigned int SampleRate, ChannelCount, PeriodTime, SampleCount;
                snd_pcm_status_t *PCMStatus = 0;
                snd_pcm_t* PCMHandle = 0;
                snd_pcm_hw_params_t *PCMParams = 0;
                snd_pcm_uframes_t PeriodSize = 0;
                snd_pcm_uframes_t PCMBufferSize = 0;
                
                linux_init_alsa_result ALSAInit = LinuxInitALSA();
                PCMHandle = ALSAInit.PCMHandle;
                PCMParams = ALSAInit.PCMParams;
                snd_pcm_hw_params_get_channels(PCMParams, &ChannelCount);
                snd_pcm_hw_params_get_rate(PCMParams, &SampleRate, 0);
                snd_pcm_hw_params_get_period_size(PCMParams, &PeriodSize, 0);
                snd_pcm_hw_params_get_period_time(PCMParams, &PeriodTime, NULL);
                snd_pcm_hw_params_get_buffer_size(PCMParams, &PCMBufferSize);
                snd_pcm_status_malloc(&PCMStatus);
                
#if 0
                {
                    Assert(0);
                    u32 Value, Result;
                    snd_pcm_uframes_t Frames;
                    Result = snd_pcm_hw_params_get_buffer_time_min(PCMParams, &Value, 0);
                    Result = snd_pcm_hw_params_get_buffer_size_min(PCMParams, &Frames);
                    Frames = PCMBufferSize/2;
                    Result = snd_pcm_hw_params_set_period_size_near(PCMHandle, PCMParams, &Frames, 0);
                    Result = snd_pcm_hw_params_get_period_size(PCMParams, &PeriodSize, 0);
                }
#endif
                
                
                char AudioSamples[PCMBufferSize];
                u64 Periods = 2;
                u32 BytesPerSample = (sizeof(s16)*ChannelCount);
                
#if 1              
                r32 GameUpdateHz = 60;
#else
                r32 GameUpdateHz = LinuxGetMonitorRefreshRate(DisplayHandle, RootWindow);
#endif
                
                thread_context ThreadContext = {};
                
                XMapWindow(DisplayHandle, WindowHandle);
                XFlush(DisplayHandle);
                
                struct timespec LastCounter = LinuxGetWallClock();
                struct timespec FlipWallClock = LinuxGetWallClock();
                r32 TargetSecondsPerFrame = 1.0f / GameUpdateHz; 
                
                GlobalRunning = true;
                
                u64 LastCycleCount = __rdtsc();
                while(GlobalRunning)
                {
                    NewInput->dtForFrame = TargetSecondsPerFrame;
                    
#if HANDMADE_INTERNAL
                    // NOTE(luca): Because gcc will first create an empty file and then write into it we skip trying to reload when the file is empty.
                    struct stat FileStats = {};
                    stat(LibraryFullPath, &FileStats);
                    if(FileStats.st_size)
                    {
                        s64 SecondsElapsed = LinuxGetNSecondsElapsed(Game.LibraryLastWriteTime, FileStats.st_mtim) / 1000/1000;
                        if(SecondsElapsed > 0)
                        {
                            LinuxUnloadGameCode(&Game);
                            Game = LinuxLoadGameCode(LibraryFullPath);
                            Game.LibraryLastWriteTime = FileStats.st_mtim;
                        }
                    }
#endif
                    
                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    NewKeyboardController->IsConnected = true;
                    
                    LinuxProcessPendingMessages(DisplayHandle, WindowHandle, WM_DELETE_WINDOW, &LinuxState, NewKeyboardController);
                    
                    // TODO(luca): Use buttonpress/release events instead so we query this less frequently.
                    s32 MouseX = 0, MouseY = 0, MouseZ = 0; // TODO(luca): Support mousewheel?
                    u32 MouseMask = 0;
                    u64 Ignored;
                    if(XQueryPointer(DisplayHandle, WindowHandle, 
                                     &Ignored, &Ignored, (int *)&Ignored, (int *)&Ignored,
                                     &MouseX, &MouseY, &MouseMask))
                    {
                        if(MouseX <= OffscreenBuffer.Width &&
                           MouseX >= 0 &&
                           MouseY <= OffscreenBuffer.Height &&
                           MouseY >= 0)
                        {
                            NewInput->MouseY = MouseY;
                            NewInput->MouseX = MouseX;
                            
                            for(u32 ButtonIndex = 0;
                                ButtonIndex < PlatformMouseButton_Count;
                                ButtonIndex++)
                            {
                                NewInput->MouseButtons[ButtonIndex].EndedDown = OldInput->MouseButtons[ButtonIndex].EndedDown;
                                NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
                            }
                            
                            LinuxProcessKeyPress(&NewInput->MouseButtons[PlatformMouseButton_Left], (MouseMask & Button1Mask));
                            LinuxProcessKeyPress(&NewInput->MouseButtons[PlatformMouseButton_Middle], (MouseMask & Button2Mask));
                            LinuxProcessKeyPress(&NewInput->MouseButtons[PlatformMouseButton_Right], (MouseMask & Button3Mask));
                        }
                    }
                    
                    for(int GamePadIndex = 0;
                        GamePadIndex < MAX_PLAYER_COUNT;
                        GamePadIndex++)
                    {
                        linux_gamepad *GamePadAt = &GamePads[GamePadIndex];
                        
                        if(GamePadAt->File != -1)
                        {
                            game_controller_input *OldController = GetController(OldInput, 0);
                            game_controller_input *NewController = GetController(NewInput, 0);
                            
                            // TODO(luca): Cross frame values!!!
                            struct input_event InputEvents[64] = {};
                            int BytesRead = 0;
                            
                            BytesRead = read(GamePadAt->File, InputEvents, sizeof(InputEvents));
                            if(BytesRead != -1)
                            {
                                for(u32 EventIndex = 0;
                                    EventIndex < ArrayCount(InputEvents);
                                    EventIndex++)
                                {
                                    struct input_event EventAt = InputEvents[EventIndex];
                                    
                                    switch(EventAt.type)
                                    {
                                        case EV_KEY:
                                        {
                                            b32 IsDown = EventAt.value;
                                            if(0) {}
                                            else if(EventAt.code == BTN_A)
                                            {
                                                NewController->ActionDown.EndedDown = IsDown;
                                            }
                                            else if(EventAt.code == BTN_B)
                                            {
                                                NewController->ActionRight.EndedDown = IsDown;
                                            }
                                            else if(EventAt.code == BTN_X)
                                            {
                                                NewController->ActionLeft.EndedDown = IsDown;
                                            }
                                            else if(EventAt.code == BTN_Y)
                                            {
                                                NewController->ActionUp.EndedDown = IsDown;
                                            }
                                            else if(EventAt.code == BTN_START)
                                            {
                                                NewController->Start.EndedDown = IsDown;
                                            }
                                            else if(EventAt.code == BTN_BACK)
                                            {
                                                NewController->Back.EndedDown = IsDown;
                                            }
                                        } break;
                                        
                                        case EV_ABS:
                                        {
                                            if(0) {}
                                            else if(EventAt.code == ABS_X)
                                            {
                                                NewController->IsAnalog = true;
                                                
                                                NewController->StickAverageX = LinuxNormalizeAxisValue(EventAt.value, GamePadAt->Axes[LSTICKX]);
                                            }
                                            else if(EventAt.code == ABS_Y)
                                            {
                                                NewController->StickAverageY = -1.0f * LinuxNormalizeAxisValue(EventAt.value, GamePadAt->Axes[LSTICKY]);
                                            }
                                            else if(EventAt.code == ABS_HAT0X)
                                            {
                                                NewController->StickAverageX = EventAt.value;
                                                NewController->IsAnalog = false;
                                            }
                                            else if(EventAt.code == ABS_HAT0Y)
                                            {
                                                NewController->StickAverageY = -EventAt.value;
                                                NewController->IsAnalog = false;
                                            }
                                        } break;
#if 0
                                        if(EventAt.type) printf("%d %d %d\n", EventAt.type, EventAt.code, EventAt.value);
#endif
                                    }
                                }
                                
                            }
                        }
                    }
                    
                    if(!GlobalPaused)
                    {
                        if(LinuxState.InputRecordingIndex)
                        {
                            LinuxRecordInput(&LinuxState, NewInput);
                        }
                        if(LinuxState.InputPlayingIndex)
                        {
                            LinuxPlayBackInput(&LinuxState, NewInput);
                        }
                        
                        // NOTE(luca): Clear buffer
                        MemSet(WindowBuffer, 0, WindowBufferSize);
                        if(Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&ThreadContext, &GameMemory, NewInput, &OffscreenBuffer);
                        }
                        
                        /* NOTE(luca): How sound works

Check the delay
Check the available frames in buffer

1. Too few audio frames in buffer for current frame.
-> Add more

2. Too many frames available
-> Add less / drain?

3. Fill first time?
-> Check delay
-> Maybe we should do this every frame?
-> Output needed frames to not have lag, this means to output maybe two frames?

TODO
- check if delay never changes
- check if buffersize never changes
-> cache these values
*/
                        
                        
                        r32 SamplesToWrite = 0;
                        local_persist b32 AudioFillFirstTime = true;
                        
                        r32 SingleFrameOfAudioFrames = TargetSecondsPerFrame*SampleRate;
                        
                        if(AudioFillFirstTime)
                        {
                            struct timespec WorkCounter = LinuxGetWallClock();
                            r32 WorkSecondsElapsed = LinuxGetSecondsElapsed(LastCounter, WorkCounter);
                            r32 SecondsLeftUntilFlip = TargetSecondsPerFrame - WorkSecondsElapsed;
                            
                            if(SecondsLeftUntilFlip > 0)
                            {
                                SamplesToWrite = SampleRate*(TargetSecondsPerFrame + SecondsLeftUntilFlip);
                            }
                            else
                            {
                                SamplesToWrite = SingleFrameOfAudioFrames;
                            }
                            
                            AudioFillFirstTime = false;
                        }
                        else
                        {
                            SamplesToWrite = SingleFrameOfAudioFrames;
                        }
                        
                        
                        game_sound_output_buffer SoundBuffer = {};
                        SoundBuffer.SamplesPerSecond = SampleRate;
                        SoundBuffer.SampleCount = SamplesToWrite;
                        SoundBuffer.Samples = (s16 *)AudioSamples;
                        
                        if(Game.GetSoundSamples)
                        {
                            Game.GetSoundSamples(&ThreadContext, &GameMemory, &SoundBuffer);
                        }
                        
#if 0
                        snd_pcm_sframes_t AvailableFrames = 0;
                        snd_pcm_sframes_t DelayFrames;
                        
                        //snd_pcm_avail_update(PCMHandle);
                        AvailableFrames = snd_pcm_avail(PCMHandle);
                        snd_pcm_delay(PCMHandle, &DelayFrames);
                        
                        //printf("PeriodSize: %lu, PeriodTime: %d, BufferSize: %lu\n", PeriodSize, PeriodTime, PCMBufferSize);
                        printf("BeingWritten: %lu, Avail: %ld, Delay: %ld, ToWrite: %d\n", PCMBufferSize - AvailableFrames, AvailableFrames, DelayFrames, (s32)SamplesToWrite);
#endif
                        
                        LastFramesWritten = snd_pcm_writei(PCMHandle, SoundBuffer.Samples, SoundBuffer.SampleCount);
                        
                        if(LastFramesWritten < 0)
                        {
                            // TODO(luca): Logging
                            // NOTE(luca): We might want to silence in case of overruns ahead of time.  We also probably want to handle latency differently here. 
                            snd_pcm_recover(PCMHandle, LastFramesWritten, 0);
                            
                            // underrun
                            if(LastFramesWritten == -EPIPE)
                            {
                                AudioFillFirstTime = true;
                            }
                            
                        }
                    }
                    
#if 0                    
                    printf("Expected: %d, Delay: %4d, Being written: %5lu, Written: %d, Ptr: %lu\n", 
                           (int)SamplesToWrite, DelayFrames, PCMBufferSize - AvailableFrames, LastFramesWritten, PCMSoundStatus->hw_ptr);
#endif
                    
                    struct timespec WorkCounter = LinuxGetWallClock();
                    r32 SecondsElapsedForFrame = LinuxGetSecondsElapsed(LastCounter, WorkCounter);
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        
                        s64 SleepUS = (s64)((TargetSecondsPerFrame - 0.001 - SecondsElapsedForFrame)*1000000.0f);
                        if(SleepUS > 0)
                        {
                            usleep(SleepUS);
                        }
                        else
                        {
                            // TODO(luca): Logging
                        }
                        
                        r32 TestSecondsElapsedForFrame = (r32)(LinuxGetSecondsElapsed(LastCounter, LinuxGetWallClock()));
                        if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            // TODO(luca): Log missed sleep
                        }
                        
                        // NOTE(luca): This is to help against sleep granularity.
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            SecondsElapsedForFrame = LinuxGetSecondsElapsed(LastCounter, LinuxGetWallClock());
                        }
                    }
                    else
                    {
                        // TODO(luca): Log missed frame rate!
                    }
                    
                    struct timespec EndCounter = LinuxGetWallClock();
                    r32 MSPerFrame = (r32)(LinuxGetNSecondsElapsed(LastCounter, EndCounter)/1000000.f);
                    LastCounter = EndCounter;
                    
                    XPutImage(DisplayHandle, WindowHandle, DefaultGC, WindowImage, 0, 0, 
                              0, 0, 
                              Width, Height);
                    FlipWallClock = LinuxGetWallClock();
                    
                    game_input *TempInput = NewInput;
                    NewInput = OldInput;
                    TempInput = NewInput;
                    
#if 0            
                    u64 EndCycleCount = __rdtsc();
                    u64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;
                    
                    r64 FPS = 0;
                    r64 MCPF = (r64)(CyclesElapsed/(1000.0f*1000.0f));
                    printf("%.2fms/f %.2ff/s %.2fmc/f\n", MSPerFrame, FPS, MCPF);
#endif
                    
                }
                
            }
            else
            {
                // TODO: Log this bad WindowHandle
            }
        }
        else
        {
            // TODO: Log this could not match visual info
        }
    }
    else
    {
        // TODO: Log could not get x connection 
    }
    return 0;
}
