#include "handmade.h"
#include "handmade_random.h"
#include "handmade_graph.cpp"

#if 1
#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"
#endif

#if HANDMADE_INTERNAL
#include <stdio.h>
#endif

internal s16
GetSineSound(u32 SampleRate)
{
    s16 Result = 0;
    
    s16 ToneVolume = (1 << 15) * 0.01;
    r32 ToneHz = 440;
    s32 WavePeriod = 0;
    local_persist r32 tSine = 0;
    WavePeriod = SampleRate/ToneHz;
    r32 SineValue = Sin(tSine);
    tSine += 2.0f*Pi32*1.0f/(r32)WavePeriod;
    if(tSine > 2*Pi32)
    {
        tSine -= 2*Pi32;
    }
    
    Result = (s16)(SineValue * ToneVolume);
    
    return Result;
}

internal void
GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer)
{
    s16 SampleValue = 0;
    
    s16 *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0;
        SampleIndex < SoundBuffer->SampleCount;
        ++SampleIndex)
    {
        
#if 0 
        SampleValue = GetSineSound(SoundBuffer->SamplesPerSecond);
#endif
        
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
    }
}

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    // TODO(casey): Let's see what the optimizer does
    
    u8 *Row = (u8 *)Buffer->Memory;    
    for(int Y = 0;
        Y < Buffer->Height;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(int X = 0;
            X < Buffer->Width;
            ++X)
        {
            u8 Blue = (u8)(X + BlueOffset);
            u8 Green = (u8)(Y + GreenOffset);
            
            *Pixel++ = (Green | Blue);
        }
        
        Row += Buffer->Pitch;
    }
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              v2 vMin, v2 vMax,
              r32 R, r32 G, r32 B)
{
    int MinX = RoundReal32ToInt32(vMin.X);
    int MaxX = RoundReal32ToInt32(vMax.X);
    int MinY = RoundReal32ToInt32(vMin.Y);
    int MaxY = RoundReal32ToInt32(vMax.Y);
    
    if(MinX < 0)
    {
        MinX = 0;
    }
    
    if(MinY < 0)
    {
        MinY = 0;
    }
    
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }
    
    u32 Color = 
    (RoundReal32ToUInt32(R * 255.0f) << 2*8) | 
    (RoundReal32ToUInt32(G * 255.0f) << 1*8) |
    (RoundReal32ToUInt32(B * 255.0f) << 0*8);
    
    u8 *Row = ((u8 *)(Buffer->Memory) + 
               MinX*Buffer->BytesPerPixel + 
               MinY*Buffer->Pitch);
    
    for(int Y = MinY;
        Y < MaxY;
        Y++)
    {
        u32 *Pixel = (u32 *)Row;
        
        for(int X = MinX;
            X < MaxX;
            X++)
        {
            *Pixel++ = Color;
        }
        
        Row += Buffer->Pitch;
    }
    
}

internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap Bitmap, 
           r32 RealX, r32 RealY,
           s32 AlignX = 0, s32 AlignY = 0)
{
    RealX -= AlignX;
    RealY -= AlignY;
    
    s32 MinX = RoundReal32ToInt32(RealX);
    s32 MinY = RoundReal32ToInt32(RealY);
    s32 MaxX = RoundReal32ToInt32(RealX + (r32)Bitmap.Width);
    s32 MaxY = RoundReal32ToInt32(RealY + (r32)Bitmap.Height);
    
    s32 SourceOffsetX = 0;
    if(MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }
    
    s32 SourceOffsetY = 0;
    if(MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }
    
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }
    
    // TODO(casey): Source row needs to be changed based on clipping.
    
    u32 *SourceRow = ((u32 *)Bitmap.Pixels + Bitmap.Width*(Bitmap.Height - 1));
    SourceRow += -(Bitmap.Width*SourceOffsetY) + SourceOffsetX;
    u8 *DestRow = ((u8 *)Buffer->Memory + 
                   MinX*Buffer->BytesPerPixel + 
                   MinY*Buffer->Pitch);
    for(s32 Y = MinY;
        Y < MaxY;
        Y++)
    {
        u32 *Dest = (u32 *)DestRow;
        u32 *Source = (u32 *)SourceRow;
        for(s32 X = MinX;
            X < MaxX;
            X++)
        {
            
            // simple alpha rendering 
            r32 A = (r32)((*Source >> 24) & 0xFF)/255.0f;
            r32 SR = (r32)((*Source >> 16) & 0xFF);
            r32 SG = (r32)((*Source >> 8) & 0xFF);
            r32 SB = (r32)((*Source >> 0) & 0xFF);
            
            r32 DR = (r32)((*Dest >> 16) & 0xFF);
            r32 DG = (r32)((*Dest >> 8) & 0xFF);
            r32 DB = (r32)((*Dest >> 0) & 0xFF);
            
            r32 R = ((1-A)*DR + A*SR);
            r32 G = ((1-A)*DG + A*SG);
            r32 B = ((1-A)*DB + A*SB);
            
            u32 C = (((u32)(R + 0.5f) << 16) |
                     ((u32)(G + 0.5f) << 8) |
                     ((u32)(B + 0.5f) << 0));
            *Dest = C;
            
            Dest++;
            Source++;
        }
        
        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap.Width;
    }
    
}

internal inline
void MemCpy(char *Dest, char *Source, size_t Count)
{
    while(Count--) *Dest++ = *Source++;
}

#pragma pack(push, 1)
struct bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    s32 Width;
    s32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 PicSize;
    u32 HorizontalResolution;
    u32 VerticalResolution;
    u32 Colors;
    u32 ColorsImportant;
    
    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
    u32 AlphaMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *DEBUGPlatformReadEntireFile,
             char *FileName)
{
    loaded_bitmap Result = {};
    debug_read_file_result File = DEBUGPlatformReadEntireFile(Thread, FileName);
    
    if(File.ContentsSize)
    {
        bitmap_header *Header = (bitmap_header *)File.Contents;
        
        u32 *Pixels = (u32 *)((u8 *)File.Contents + Header->BitmapOffset);
        
        // NOTE(casey): If you are using this generically for some reason, please remember that
        // BMP files CAN GO IN EITHER DIRECTION and the height will be negative for top-down.
        // (Also, there can be compression, etc..., etc...)
        
        // NOTE(casey): Byte order in memory is determined by the header itself, so we have to read
        // out the masks and convert the pixels ourselves.
        s32 RedMask = Header->RedMask;
        s32 BlueMask = Header->BlueMask;
        s32 GreenMask = Header->GreenMask;
        s32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
        
        bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);
        Assert(AlphaShift.Found);
        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(Header->Compression == 3);
        
        u32 *SourceDest = Pixels;
        for(s32 Y = 0;
            Y < Header->Height;
            Y++)
        {
            for(s32 X = 0;
                X < Header->Width;
                X++)
            {
                u32 C = *SourceDest;
                u32 ConvertedColor = ((((C >> AlphaShift.Index) & 0xFF) << 24) | 
                                      (((C >> RedShift.Index)   & 0xFF) << 16) | 
                                      (((C >> GreenShift.Index) & 0xFF) << 8)  | 
                                      (((C >> BlueShift.Index)  & 0xFF) << 0));
                *SourceDest++ = ConvertedColor;
            }
        }
        
        Result.Pixels = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
    }
    
    return Result;
}

internal void
DrawVerticalAxis(game_offscreen_buffer *Buffer, view *View, r32 PointX)
{
    PointX += View->CenterOffset.X;
    
    for(u32 Y = 0;
        Y < View->SizePixels.Y;
        Y++)
    {
        u32 X = PointX*View->PointsToPixels + View->TopLeft.X + View->PointPad.X;
        
        v2 LineMin = {(r32)X, (r32)Y + View->TopLeft.Y};
        v2 LineMax = LineMin + v2{1, 1};
        
        DrawRectangle(Buffer, LineMin, LineMax, 0.0f, 0.0f, 1.0f);
        
    }
}

internal void
DrawHorizontalAxis(game_offscreen_buffer *Buffer, view *View, r32 PointY)
{
    PointY += View->CenterOffset.Y;
    
    for(u32 X = 0;
        X < View->SizePixels.X;
        X++)
    {
        u32 Y = PointY*View->PointsToPixels + View->TopLeft.Y + View->PointPad.Y;
        
        v2 LineMin = {(r32)X + View->TopLeft.X, (r32)Y};
        v2 LineMax = LineMin + v2{1, 1};
        
        DrawRectangle(Buffer,
                      LineMin, LineMax,
                      1.0f, 1.0f, 0.0f);
    }
    
}

internal void
DrawPoint(game_offscreen_buffer *Buffer, view *View, v2 Point, color_rgb Color, v2 PointSize = {1, 1})
{
    Point.Y *= -1;
    Point += View->CenterOffset;
    v2 PointMin = Point*View->PointsToPixels + View->TopLeft + View->PointPad;
    v2 PointMax = PointMin + v2{1, 1};
    PointMin += -PointSize;
    PointMax += PointSize;
    
    b32 IsOutOfView = ((PointMin.X < View->TopLeft.X) ||
                       (PointMin.Y < View->TopLeft.Y) || 
                       (PointMax.X > View->BottomRight.X) ||
                       (PointMax.Y > View->BottomRight.Y)); 
    if(!IsOutOfView)
    {
        DrawRectangle(Buffer, PointMin, PointMax,
                      Color.R, Color.G, Color.B);
    }
    
}

internal void
DrawLine(game_offscreen_buffer *Buffer, view *View, 
         r32 Slope, r32 B, r32 Step, 
         color_rgb Color)
{
    
    for(r32 X = -(View->CenterOffset.X + 1);
        X < (View->CenterOffset.X + 1);
        X += Step)
    {
        r32 Y = X * Slope + B;
        DrawPoint(Buffer, View, v2{X, Y}, Color, {.5, .5});
    }
    
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    local_persist stbtt_fontinfo FontInfo = {};
    local_persist unsigned char *Bitmap = 0;
    local_persist int FontHeight, FontWidth;
    if(!Memory->IsInitialized)
    {
        GameState->Slope = 1.0f;
        GameState->Step = 0.5f;
        
        debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Thread, "data/font.ttf");
        if(stbtt_InitFont(&FontInfo, (unsigned char*)File.Contents, 0))
        {
            int C = 'a';
            float HeightPixels = 64;
            float ScaleY = stbtt_ScaleForPixelHeight(&FontInfo, HeightPixels);
            
#if 1
            Bitmap = stbtt_GetCodepointBitmap(&FontInfo, 0, ScaleY, C, &FontWidth, &FontHeight, 0, 0);
#else
            int Width = 100;
            int Height = 100;
            unsigned char Bitmap[Width*Height];
            stbtt_MakeCodepointBitmap(&FontInfo, Bitmap, Width, Height, Width, 0, ScaleY, 'a');
#endif
            
        }
        else
        {
            Assert(0);
        }
        
        Memory->IsInitialized = true;
        
    }
    
    {
        
        u8 *Row = (u8 *)(Buffer->Memory);
        // Rendering the font?
        for(int  Y = 0;
            Y < FontHeight;
            Y++)
        {
            u32 *Pixel = (u32 *)Row;
            for(int X = 0;
                X < FontWidth;
                X++)
            {
                u8 Brightness = Bitmap[Y*FontWidth+X];
                u32 Color = ((0xFF << 24) |
                             (Brightness << 16) |
                             (Brightness << 8) |
                             (Brightness << 0));
                *Pixel++ = Color;
            }
            Row += Buffer->Pitch;
        }
        
    }
    
    
    v2 BufferSize = {(r32)Buffer->Width, (r32)Buffer->Height};
    
    view View = {};
    View.PointsToPixels = 20;
    View.SizePixels = {(r32)(Buffer->Width), (r32)(Buffer->Height)};
    // NOTE(luca): Create a square based on 16/9 aspect ratio.
    View.SizePixels.X *= (1.00f - 7.0f/16.0f);
    // NOTE(luca): This is truncated so that when it is scaled back to pixels we can use the "lost" pixels for centering.  On top of that we make sure that it is an even number so both axises' middle align with the center..
    View.SizePoints = V2(((r32)(((u32)View.SizePixels.X / View.PointsToPixels) & (~1))),
                         ((r32)(((u32)View.SizePixels.Y / View.PointsToPixels) & (~1)))); 
    View.CenterOffset = 0.5f*View.SizePoints;
    View.TopLeft = (BufferSize - View.SizePixels)/2.0f; 
    View.BottomRight = View.TopLeft + View.SizePixels;
    View.PointPad = 0.5f*(View.SizePixels - (r32)View.PointsToPixels*View.SizePoints);
    
    for(u32 ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if(Controller->IsConnected)
        {
            
            if(Controller->IsAnalog)
            {
                
            }
            else
            {
                r32 SlopeStep = 1.0f*Input->dtForFrame;
                r32 StepStep = 1.0f*Input->dtForFrame;
                r32 BStep = 1.0f*Input->dtForFrame;
                r32 CStep = 8.0f*Input->dtForFrame;
                
                if(Controller->MoveUp.EndedDown)
                {
                    GameState->Slope += SlopeStep;
                }
                
                if(Controller->MoveDown.EndedDown)
                {
                    GameState->Slope -= SlopeStep;
                }
                
                if(Controller->MoveRight.EndedDown)
                {
                    GameState->Step -= StepStep;
                    
                }
                if(Controller->MoveLeft.EndedDown)
                {
                    GameState->Step += StepStep;
                }
                
                if(GameState->Step <= 0)
                {
                    GameState->Step = 0.001;
                }
                
                if(Controller->ActionUp.EndedDown)
                {
                    GameState->C += CStep;
                }
                if(Controller->ActionDown.EndedDown)
                {
                    GameState->C -= CStep;
                }
                
                if(Controller->ActionRight.EndedDown)
                {
                    GameState->B += BStep;
                }
                if(Controller->ActionLeft.EndedDown)
                {
                    GameState->B -= BStep;
                }
                
                if(Controller->Start.EndedDown)
                {
                    GameState->Step = 0.01f;
                    GameState->Slope = 1.0f;
                    GameState->B = 0.0f;
                    GameState->C = 0.0f;
                }
            };
        }
    }
    
    
    //-Rendering 
    
    DrawRectangle(Buffer, View.TopLeft, View.BottomRight, 0.1f, 0.1f, 0.1f);
    DrawHorizontalAxis(Buffer, &View, 0);
    DrawVerticalAxis(Buffer, &View, 0);
    
    // X Points
    for(r32 Col = 0;
        Col <= View.SizePoints.X;
        Col++)
    {
        v2 LineMin = v2{Col, View.CenterOffset.Y - 0.5f};
        v2 LineMax = v2{Col, View.CenterOffset.Y + 0.5f};
        LineMin = (LineMin*View.PointsToPixels) + View.TopLeft + View.PointPad;
        LineMax = (LineMax*View.PointsToPixels) + View.TopLeft + View.PointPad + v2{1, 1};
        
        DrawRectangle(Buffer, LineMin, LineMax, 1.0f, 1.0f, 1.0f);
    }
    
    // Y Points
    for(r32 Row = 0;
        Row <= View.SizePoints.Y;
        Row++)
    {
        v2 LineMin = v2{View.CenterOffset.X - 0.5f, Row};
        v2 LineMax = v2{View.CenterOffset.X + 0.5f, Row};
        LineMin = (LineMin*View.PointsToPixels) + View.TopLeft + View.PointPad;
        LineMax = (LineMax*View.PointsToPixels) + View.TopLeft + View.PointPad + v2{1, 1};
        DrawRectangle(Buffer, LineMin, LineMax, 1.0f, 1.0f, 1.0f);
    }
    
    
    // Plot some points
    r32 B = GameState->B;
    r32 C = GameState->C;
    r32 Slope = GameState->Slope;
    r32 Step = GameState->Step;
    
#if 0    
    DrawLine(Buffer, &View, Slope, B, Step, {1.0f, 0.0f, 1.0f}); 
#endif
    
    for(r32 X = -(View.CenterOffset.X + 1);
        X < (View.CenterOffset.X + 1);
        X += Step)
    {
        r32 Y = 16.0f*Slope*Sin(X + -4*B) + 2*C;
        DrawPoint(Buffer, &View, v2{X, Y}, color_rgb{1.0f, 0.5f, 0.0f}, {.5, .5});
    }
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer);
}