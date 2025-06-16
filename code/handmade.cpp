#include "handmade.h"
#include "handmade_random.h"
#include "handmade_graph.cpp"

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
DrawVerticalAxis(game_offscreen_buffer *Buffer, 
                 u32 ViewHeight, u32 PointsToPixels, v2 Pad, v2 PointPad, v2 PointCenterOffset,
                 r32 PointX)
{
    PointX += PointCenterOffset.X;
    
    for(u32 Y = 0;
        Y < ViewHeight;
        Y++)
    {
        u32 X = PointX*PointsToPixels + PointPad.X;
        
        v2 LineMin = {(r32)X, (r32)Y + Pad.Y};
        v2 LineMax = LineMin + v2{1, 1};
        
        DrawRectangle(Buffer,
                      LineMin, LineMax,
                      0.0f, 0.0f, 1.0f);
        
    }
}

internal void
DrawHorizontalAxis(game_offscreen_buffer *Buffer,
                   u32 ViewWidth, u32 PointsToPixels, v2 Pad, v2 PointPad, v2 PointCenterOffset,
                   r32 PointY)
{
    PointY += PointCenterOffset.Y;
    
    for(u32 X = 0;
        X < ViewWidth;
        X++)
    {
        u32 Y = PointY*PointsToPixels + PointPad.Y;
        
        v2 LineMin = {(r32)X + Pad.X, (r32)Y};
        v2 LineMax = LineMin + v2{1, 1};
        
        DrawRectangle(Buffer,
                      LineMin, LineMax,
                      1.0f, 1.0f, 0.0f);
    }
    
}

internal void
DrawPoint(game_offscreen_buffer *Buffer,
          v2 TopLeft, v2 BottomRight,
          u32 PointsToPixels, v2 Pad, v2 PointPad, v2 PointCenterOffset,
          v2 Point, v2 PointSize = {1, 1})
{
    Point.Y *= -1;
    Point += PointCenterOffset;
    v2 PointMin = Point*PointsToPixels + PointPad;
    v2 PointMax = PointMin + v2{1, 1};
    PointMin += -PointSize;
    PointMax += PointSize;
    
    b32 IsOutOfView = ((PointMin.X < TopLeft.X) ||
                       (PointMin.Y < TopLeft.Y) || 
                       (PointMax.X > BottomRight.X) ||
                       (PointMax.Y > BottomRight.Y)); 
    if(!IsOutOfView)
    {
        DrawRectangle(Buffer, PointMin, PointMax,
                      1.0f, 0.0f, 1.0f);
    }
    
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        Memory->IsInitialized = true;
        GameState->Slope = 1.0f;
        GameState->Step = 0.5f;
    }
    
    u32 PointsToPixels = 20;
    v2 ViewSizePixels = V2((r32)(0.56f*Buffer->Width), 
                           (r32)(1.00f*Buffer->Height));
    // NOTE(luca): This is truncated so that when it is scaled back to pixels we can use the "lost" pixels for centering.  On top of that we make sure that it is an even number so both axises pass through {0, 0}.
    
    v2 ViewSizePoints = V2(((r32)(((u32)ViewSizePixels.X / PointsToPixels) & (0xFFFFFFFF - 1))),
                           ((r32)(((u32)ViewSizePixels.Y / PointsToPixels) & (0xFFFFFFFF - 1)))); 
    
    v2 ScreenCenter = 0.5f*ViewSizePixels;
    
    // NOTE(luca): This value is used to interpret points as being centered.
    v2 PointCenterOffset = 0.5f*ViewSizePoints;
    
    // NOTE(luca): Make it so that we can only draw in a restricted area, this is useful so we can change the scale in both dimensions.
    
    v2 BufferSize = {(r32)Buffer->Width, (r32)Buffer->Height};
    v2 Pad = (BufferSize - ViewSizePixels)/2.0f; 
    
    // NOTE(luca): We need to add padding to center points in the restricted area
    v2 PointPad = Pad + 0.5f*(ViewSizePixels - (r32)PointsToPixels*ViewSizePoints);
    v2 TopLeft = Pad;
    v2 BottomRight = Pad + ViewSizePixels;
    
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
                
                r32 SlopeStep = 2.0f*Input->dtForFrame;
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
                    GameState->Step -= 0.02f;
                    
                }
                
                if(Controller->MoveLeft.EndedDown)
                {
                    GameState->Step += 0.02f;
                }
                if(GameState->Step <= 0)
                {
                    GameState->Step = 0.001;
                }
                
                if(Controller->ActionUp.EndedDown)
                {
                    GameState->Step = 0.5f;
                    GameState->Slope = 1.0f;
                }
            };
        }
    }
    
    
    //-Rendering 
    
    DrawRectangle(Buffer, TopLeft, BottomRight, 0.1f, 0.1f, 0.1f);
    DrawHorizontalAxis(Buffer, ViewSizePixels.X, PointsToPixels, Pad, PointPad, PointCenterOffset, 0);
    DrawVerticalAxis(Buffer, ViewSizePixels.Y, PointsToPixels, Pad, PointPad, PointCenterOffset, 0);
    
    // X Points
    for(r32 Col = 0;
        Col <= ViewSizePoints.X;
        Col++)
    {
        v2 LineMin = {Col, PointCenterOffset.Y - 0.5f};
        v2 LineMax = {Col, PointCenterOffset.Y + 0.5f};
        
        DrawRectangle(Buffer, 
                      PointPad + LineMin*PointsToPixels, 
                      PointPad + LineMax*PointsToPixels + v2{1, 1}, 
                      1.0f, 1.0f, 1.0f);
    }
    
    // Y Points
    for(r32 Row = 0;
        Row <= ViewSizePoints.Y;
        Row++)
    {
        v2 LineMin = v2{PointCenterOffset.X - 0.5f, Row};
        v2 LineMax = v2{PointCenterOffset.X + 0.5f, Row};
        DrawRectangle(Buffer, 
                      PointPad + LineMin*PointsToPixels , 
                      PointPad + LineMax*PointsToPixels + v2{1, 1},
                      1.0f, 1.0f, 1.0f);
    }
    
    
    // Plot some points
    
    r32 B = 0.0f;
    r32 Step =  GameState->Step;
    r32 Slope = GameState->Slope;
    
    for(r32 X = -(PointCenterOffset.X + 1);
        X < (PointCenterOffset.X + 1);
        X += Step)
    {
        r32 Y = X * Slope + B;
        DrawPoint(Buffer, 
                  TopLeft, BottomRight, 
                  PointsToPixels, Pad, PointPad, PointCenterOffset, 
                  V2(X, Y), {.5, .5});
    }
    
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer);
}