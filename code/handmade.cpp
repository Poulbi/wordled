#include "handmade.h"
#include "handmade_random.h"
#include "handmade_graph.cpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

// TODO(luca): Get rid of these.
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

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
              color_rgb Color)
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
    
    u32 ColorValue = 
    (RoundReal32ToUInt32(Color.R * 255.0f) << 2*8) | 
    (RoundReal32ToUInt32(Color.G * 255.0f) << 1*8) |
    (RoundReal32ToUInt32(Color.B * 255.0f) << 0*8);
    
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
            *Pixel++ = ColorValue;
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
DrawCharacter(game_offscreen_buffer *Buffer,  u8 *FontBitmap,
              int FontWidth, int FontHeight, 
              int XOffset, int YOffset,
              color_rgb Color)
{
    s32 MinX = 0;
    s32 MinY = 0;
    s32 MaxX = FontWidth;
    s32 MaxY = FontHeight;
    
    if(XOffset < 0)
    {
        MinX = -XOffset;
        XOffset = 0;
    }
    if(YOffset < 0)
    {
        MinY = -YOffset;
        YOffset = 0;
    }
    if(XOffset + FontWidth > Buffer->Width)
    {
        MaxX -= ((XOffset + FontWidth) - Buffer->Width);
    }
    if(YOffset + FontHeight > Buffer->Height)
    {
        MaxY -= ((YOffset + FontHeight) - Buffer->Height);
    }
    
    u8 *Row = (u8 *)(Buffer->Memory) + 
    (YOffset*Buffer->Pitch) +
    (XOffset*Buffer->BytesPerPixel);
    
    for(int  Y = MinY;
        Y < MaxY;
        Y++)
    {
        u32 *Pixel = (u32 *)Row;
        for(int X = MinX;
            X < MaxX;
            X++)
        {
            u8 Brightness = FontBitmap[Y*FontWidth+X];
            r32 Alpha = ((r32)Brightness/255.0f);
            
            r32 DR = (r32)((*Pixel >> 16) & 0xFF);
            r32 DG = (r32)((*Pixel >> 8) & 0xFF);
            r32 DB = (r32)((*Pixel >> 0) & 0xFF);
            
            r32 R = Color.R*255.0f*Alpha + DR*(1-Alpha);
            r32 G = Color.G*255.0f*Alpha + DG*(1-Alpha);
            r32 B = Color.B*255.0f*Alpha +  DB*(1-Alpha);
            
            u32 Value = ((0xFF << 24) |
                         ((u32)(R) << 16) |
                         ((u32)(G) << 8) |
                         ((u32)(B) << 0));
            *Pixel++ = Value;
        }
        
        Row += Buffer->Pitch;
    }
}

internal void
DrawText(game_state *GameState, game_offscreen_buffer *Buffer, 
         r32 FontScale,
         rune *Text, u32 TextLen, v2 Offset, color_rgb Color)
{
    for(u32 TextIndex = 0;
        TextIndex < TextLen;
        TextIndex++)
    {
        rune CharAt = Text[TextIndex];
        
        s32 FontWidth, FontHeight;
        s32 AdvanceWidth, LeftSideBearing;
        s32 X0, Y0, X1, Y1;
        u8 *FontBitmap = 0;
        // TODO(luca): Get rid of malloc.
        FontBitmap = stbtt_GetCodepointBitmap(&GameState->FontInfo, 
                                              FontScale, FontScale, 
                                              CharAt, 
                                              &FontWidth, &FontHeight, 0, 0);
        stbtt_GetCodepointBitmapBox(&GameState->FontInfo, CharAt, 
                                    FontScale, FontScale, 
                                    &X0, &Y0, &X1, &Y1);
        r32 YOffset = Offset.Y + Y0;
        stbtt_GetCodepointHMetrics(&GameState->FontInfo, CharAt, &AdvanceWidth, &LeftSideBearing);
        r32 XOffset = Offset.X + LeftSideBearing*FontScale;
        
        DrawCharacter(Buffer, FontBitmap, FontWidth, FontHeight, XOffset, YOffset, Color);
        
        Offset.X += (AdvanceWidth*FontScale);
        free(FontBitmap);
    }
}

internal b32
ValidLetterCountInGuess(char *Word, char *Guess, char Letter)
{
    b32 Valid = false;
    
    int CountInGuess = 0;
    int CountInWord = 0;
    for(int ScanIndex = 0;
        ScanIndex < WORDLE_LENGTH;
        ScanIndex++)
    {
        if(Letter == Word[ScanIndex])
        {
            CountInWord++;
        }
        if(Letter == Guess[ScanIndex])
        {
            CountInGuess++;
        }
    }
    
    Valid = (CountInGuess <= CountInWord);
    
    return Valid;
}

internal void
GetTodaysWordle(thread_context *Thread, game_memory *Memory, char *Word)
{
    struct tm *LocalTimeNow = 0;
    time_t Now = 0;
    time(&Now);
    LocalTimeNow = localtime(&Now);
    
    char URL[] = "https://www.nytimes.com/svc/wordle/v2";
    char URLBuffer[256] = {0};
    sprintf(URLBuffer, "%s/%d-%02d-%d.json", URL, 
            LocalTimeNow->tm_year + 1900, LocalTimeNow->tm_mon + 1, LocalTimeNow->tm_mday);
    char OutputBuffer[4096] = {0};
    char *Command[] = 
    {
        "/usr/bin/curl", 
        "--silent", 
        "--location", 
        "--max-time", "10",
        URLBuffer, 0
    };
    
    // TODO(luca): This should be asynchronous at least because in the case of no internet the application would not even start.
    int BytesOutputted = Memory->PlatformRunCommandAndGetOutput(Thread, OutputBuffer, Command);
    int Matches;
    int MatchedAt = 0;
    
    for(int At = 0;
        At < BytesOutputted;
        At++)
    {
        Matches = true;
        char ScanMatch[] = "solution";
        int ScanMatchSize = sizeof(ScanMatch) - 1;
        for(int ScanAt = 0;
            ScanAt < ScanMatchSize;
            ScanAt++)
        {
            if((OutputBuffer + At)[ScanAt] != ScanMatch[ScanAt])
            {
                Matches = false;
                break;
            }
        }
        if(Matches)
        {
            MatchedAt = At;
            break;
        }
    }
    if(Matches)
    {
        int Start = 0;
        int End = 0;
        int Scan = MatchedAt;
        while(OutputBuffer[Scan++] != '"' && Scan < BytesOutputted);
        while(OutputBuffer[Scan++] != '"' && Scan < BytesOutputted);
        Start = Scan;
        while(OutputBuffer[Scan] != '"' && Scan < BytesOutputted) Scan++;
        End = Scan;
        
        MemCpy(Word, OutputBuffer+Start, End-Start);
    }
}

internal void 
AppendCharToInputText(game_state *GameState, rune Codepoint)
{
    if(GameState->TextInputCount < ArrayCount(GameState->TextInputText))
    {
        GameState->TextInputText[GameState->TextInputCount++] = Codepoint;
    }
}

internal color_rgb
GetColorRGBForColorIndex(u32 Index)
{
    color_rgb Color = {};
    color_rgb ColorGray = {0.23f, 0.23f, 0.24f};
    color_rgb ColorYellow = {0.71f, 0.62f, 0.23f};
    color_rgb ColorGreen = {0.32f, 0.55f, 0.31f};
    
    if(0) {}
    else if(Index == SquareColor_Gray)   Color = ColorGray;
    else if(Index == SquareColor_Yellow) Color = ColorYellow;
    else if(Index == SquareColor_Green)  Color = ColorGreen;
    
    return Color;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    
    if(!Memory->IsInitialized)
    {
        debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Thread, "../data/font.ttf");
        
        if(stbtt_InitFont(&GameState->FontInfo, (u8 *)File.Contents, stbtt_GetFontOffsetForIndex((u8 *)File.Contents, 0)))
        {
            GameState->FontInfo.data = (u8 *)File.Contents;
            
            int X0, Y0, X1, Y1;
            v2 BoundingBox[2] = {};
            stbtt_GetFontBoundingBox(&GameState->FontInfo, &X0, &Y0, &X1, &Y1);
            GameState->FontBoundingBox[0] = v2{(r32)X0, (r32)Y0};
            GameState->FontBoundingBox[1] = v2{(r32)X1, (r32)Y1};
            stbtt_GetFontVMetrics(&GameState->FontInfo, &GameState->FontAscent, &GameState->FontDescent, &GameState->FontLineGap);
        }
        else
        {
            // TODO(luca): Logging
        }
        
        GameState->SelectedColor = SquareColor_Yellow;
        GameState->ExportedPatternIndex = 0;
        
#if 0        
        GetTodaysWordle(Thread, Memory, GameState->WordleWord);
#else
        char *Word = "sword";
        for(u32 Count = 0; Count < 5; Count++)
        {
            GameState->WordleWord[Count] = Word[Count];
        }
#endif
        
        GameState->TextInputCount = 0;
        GameState->TextInputMode = true;
        
        Memory->IsInitialized = true;
    }
    
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
                Assert(Controller->Keyboard.TextInputCount < ArrayCount(Controller->Keyboard.TextInputBuffer));
                for(u32 InputIndex = 0;
                    InputIndex < Controller->Keyboard.TextInputCount;
                    InputIndex++)
                {
                    game_text_button Button = Controller->Keyboard.TextInputBuffer[InputIndex];
                    if(0) {}
                    else if(Button.Codepoint == 'u' && Button.Control)
                    {
                        GameState->TextInputCount = 0;
                    }
                    else if(Button.Codepoint == 'd' && Button.Control)
                    {
                        if(GameState->TextInputCount)
                        {
                            GameState->TextInputCount--;
                        }
                    }
                    else
                    {
                        AppendCharToInputText(GameState, Button.Codepoint);
                    }
                    
                }
                
                if(WasPressed(Input->MouseButtons[PlatformMouseButton_ScrollUp]))
                {
                    u32 ColorIndex = GameState->SelectedColor;
                    GameState->SelectedColor = ((ColorIndex > 0) ? 
                                                (ColorIndex - 1) : 
                                                (SquareColor_Count - 1));
                }
                if(WasPressed(Input->MouseButtons[PlatformMouseButton_ScrollDown]))
                {
                    u32 ColorIndex = GameState->SelectedColor;
                    GameState->SelectedColor = ((ColorIndex + 1 < SquareColor_Count) ?
                                                (ColorIndex + 1) :
                                                (0));
                }
                if(WasPressed(Controller->ActionLeft))
                {
                    GameState->TextInputMode = !GameState->TextInputMode;
                }
                
            }
        }
    }
    
    Assert(GameState->SelectedColor < SquareColor_Count);
    
    r32 Width = 48.0f;
    v2 Min = {};
    v2 Max = {};
    v2 Padding = {2.0f, 2.0f};
    v2 Base = {0.0f, 0.0f};
    s32 Rows = 6;
    s32 Columns = 5;
    
    Base.X = 0.5f*(Buffer->Width - Columns*Width);
    Base.Y = 0.5f*(Buffer->Height - Rows*Width);
    s32 SelectedX = CeilReal32ToInt32((r32)(Input->MouseX - Base.X)/(r32)(Width + Padding.X)) - 1;
    s32 SelectedY = CeilReal32ToInt32((r32)(Input->MouseY - Base.Y)/(r32)(Width + Padding.Y)) - 1;
    
    {    
        r32 WheelWidth = Width * 0.66f;
        v2 BorderWidth = Padding;
        Min = v2{Base.X, Base.Y - Width};
        for(u32 ColorIndex = 0;
            ColorIndex < SquareColor_Count;
            ColorIndex++)
        {
            Max = Min + WheelWidth;
            color_rgb Color = GetColorRGBForColorIndex(ColorIndex);
            
            if(ColorIndex == GameState->SelectedColor)
            {
                r32 Gray = 0.7f; 
                DrawRectangle(Buffer, Min, Max, color_rgb{Gray, Gray, Gray});
            }
            
            DrawRectangle(Buffer, 
                          Min + BorderWidth, Max - BorderWidth,
                          Color);
            
            Min.X += Padding.X + WheelWidth;
        }
    }
    
    Min = Base;
    for(s32 Y = 0;
        Y < Rows;
        Y++)
    {
        for(s32 X = 0;
            X < Columns;
            X++)
        {
            color_rgb Color = {1.0f, 1.0f, 0.0f};
            
            Max = Min + Width;
            
            if((X == SelectedX) &&
               (Y == SelectedY))
            {
                if(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                {
                    GameState->PatternGrid[Y][X] = GameState->SelectedColor;
                }
                Color = GetColorRGBForColorIndex(GameState->SelectedColor);
                DrawRectangle(Buffer, Min, Max, color_rgb{0.0f, 0.0f, 0.0f});
                DrawRectangle(Buffer, 
                              Min + Padding, Max - Padding,
                              Color);
            }
            else
            {
                u32 PatternValue = GameState->PatternGrid[Y][X];
                Color = GetColorRGBForColorIndex(PatternValue);
                DrawRectangle(Buffer, Min, Max, Color);
            }
            
            
            Min.X += Padding.X + Width;
        }
        Min.X = Base.X;
        Min.Y += Padding.Y + Width;
    }
    
    char Text[256] = {};
    int TextLen = 0;
    r32 FontScale = 0.0f;
    r32 YAdvance = 0.0f;
    r32 Baseline = 0.0f;
    v2 TextOffset = {};
    int AdvanceWidth = 0;
    
    // Prepare drawing of the guesses.
    FontScale = stbtt_ScaleForPixelHeight(&GameState->FontInfo, 24.0f);
    YAdvance = FontScale*(GameState->FontAscent - 
                          GameState->FontDescent + 
                          GameState->FontLineGap);
    Baseline = (GameState->FontAscent*FontScale);
    
    TextOffset = v2{16.0f, 16.0f + Baseline};
    
#if 0    
    {
        char Text[WORDLE_LENGTH + 2 + 1] = {};
        s32 TextLen = sprintf(Text, "\"%s\"", GameState->WordleWord); 
        DrawText(GameState, Buffer, FontScale, Text, TextLen, TextOffset + -v2{8.0f, 0.0f}, ColorYellow);
    }
#endif
    
    TextOffset.Y += YAdvance*2.0f;
    
    //-Matche the pattern
    char *Word = GameState->WordleWord;
    debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Thread, "../data/words.txt");
    
    int WordsCount = File.ContentsSize / WORDLE_LENGTH;
    if(File.Contents)
    {
        char *Words = (char *)File.Contents;
        
        int PatternRowAt = 0;
        int PatternRowsCount = 6;
        
        for(int WordsIndex = 0;
            ((WordsIndex < WordsCount) && 
             (PatternRowAt < PatternRowsCount));
            WordsIndex++)
        {
            // Match the pattern's row against the guess.
            // TODO(luca): Check if the guess == the word and skip it otherwise it would end the game.
            int PatternMatches = 1;
            char *Guess = &Words[WordsIndex*5];
            for(int CharIndex = 0;
                ((CharIndex < WORDLE_LENGTH) &&
                 (PatternMatches));
                CharIndex++)
            {
                char GuessCh = Guess[CharIndex];
                int PatternValue = GameState->PatternGrid[PatternRowAt][CharIndex];
                
                if(PatternValue == SquareColor_Green)
                {
                    PatternMatches = (GuessCh == Word[CharIndex]);
                }
                else if(PatternValue == SquareColor_Yellow)
                {
                    PatternMatches = 0;
                    for(int CharAt = 0;
                        CharAt < WORDLE_LENGTH;
                        CharAt++)
                    {
                        if(Word[CharAt] == GuessCh)
                        {
                            if(CharAt != CharIndex)
                            {
                                // TODO(luca): Should also check that position does not match.
                                PatternMatches = ValidLetterCountInGuess(Word, Guess, GuessCh);
                            }
                            else
                            {
                                PatternMatches = 0;
                            }
                            
                            break;
                        }
                    }
                    
                }
                // TODO(luca): Have one that can be either yellow/green
#if 0
                else if(PatternValue == 1)
                {
                    PatternMatches = 0;
                    for(int CharAt = 0;
                        CharAt < WORDLE_LENGTH;
                        CharAt++)
                    {
                        if(Word[CharAt] == GuessCh)
                        {
                            PatternMatches = ValidLetterCountInGuess(Word, Guess, GuessCh);
                            break;
                        }
                    }
                }
#endif
                else if(PatternValue == SquareColor_Gray)
                {
                    PatternMatches = 1;
                    for(int CharAt = 0;
                        CharAt < WORDLE_LENGTH;
                        CharAt++)
                    {
                        if(Word[CharAt] == GuessCh)
                        {
                            PatternMatches = 0;
                            break;
                        }
                    }
                }
            }
            
            if(PatternMatches)
            {
#if 0
                DrawText(GameState, Buffer, FontScale,
                         Guess, WORDLE_LENGTH, 
                         TextOffset, color_rgb{1.0f, 1.0f, 1.0f});
#endif
                
                TextOffset.Y += YAdvance;
                
                WordsIndex = 0;
                PatternRowAt++;
            }
        }
    }
    
    Memory->DEBUGPlatformFreeFileMemory(Thread, File.Contents, File.ContentsSize);
    
    // NOTE(luca): Debug code for drawing inputted text.
#if 1
    {
        r32 FontScale = stbtt_ScaleForPixelHeight(&GameState->FontInfo, 20.0f);
        
        v2 Offset = {100.0f, 30.0f};
        
        r32 TextHeight = FontScale*(GameState->FontAscent - 
                                    GameState->FontDescent + 
                                    GameState->FontLineGap);
        r32 TextWidth = 0;
        for(u32 InputIndex = 0;
            InputIndex < GameState->TextInputCount;
            InputIndex++)
        {
            int AdvanceWidth, LeftSideBearing;
            rune Codepoint = GameState->TextInputText[InputIndex];
            stbtt_GetCodepointHMetrics(&GameState->FontInfo, Codepoint,
                                       &AdvanceWidth, &LeftSideBearing);
            TextWidth += (AdvanceWidth)*FontScale;
        }
        
        r32 Baseline = (GameState->FontAscent*FontScale);
        
        v2 Min = {Offset.X, Offset.Y - Baseline};
        v2 Max = {Offset.X + TextWidth, Min.Y + TextHeight};
        color_rgb ColorBorder = {0.017f, 0.017f, 0.017f};
        color_rgb ColorBG = {0.007f, 0.007f, 0.007f};
        color_rgb ColorFG = {0.682f, 0.545f, 0.384f};
        
        DrawRectangle(Buffer, Min + -1, Max + 1, ColorBorder);
        DrawRectangle(Buffer, Min, Max, ColorBG);
        
        DrawText(GameState, Buffer, FontScale,
                 GameState->TextInputText, GameState->TextInputCount, 
                 Offset, ColorFG);
        
        Assert(GameState->TextInputCount < ArrayCount(GameState->TextInputText));
    }
    
#endif
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer);
}
