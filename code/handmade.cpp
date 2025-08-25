#include "handmade.h"
#include "handmade_random.h"

#include <curl/curl.h>
#define STB_SPRINTF_IMPLEMENTATION
#include "libs/stb_sprintf.h"

// TODO(luca): Get rid of this, do note that time.h is already included by curl.
#include <time.h>

//- Helpers 
internal color_rgb
GetColorRGBForColorIndex(u32 Index)
{
    color_rgb Result = {};
    color_rgb ColorGray = {0.23f, 0.23f, 0.24f};
    color_rgb ColorYellow = {0.71f, 0.62f, 0.23f};
    color_rgb ColorGreen = {0.32f, 0.55f, 0.31f};
    
    if(0) {}
    else if(Index == SquareColor_Gray)   Result = ColorGray;
    else if(Index == SquareColor_Yellow) Result = ColorYellow;
    else if(Index == SquareColor_Green)  Result = ColorGreen;
    
    return Result;
}

void MemoryCopy(void *Dest, void *Source, size_t Count)
{
    u8 *DestinationByte = (u8 *)Dest;
    u8 *SourceByte = (u8 *)Source;
    while(Count--) *DestinationByte++ = *SourceByte++;
}

//- Sound 
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

//- Rendering 
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
DrawText(game_offscreen_buffer *Buffer, game_font *Font, r32 FontScale,
         u32 TextLen, u8 *Text, v2 Offset, color_rgb Color, b32 IsUTF8)
{
    Assert(Font->Initialized);
    
    for(u32 TextIndex = 0;
        TextIndex < TextLen;
        TextIndex++)
    {
        rune CharAt = 0;
        if(IsUTF8)
        {
            CharAt = *(((rune *)Text) + TextIndex);
        }
        else
        {
            CharAt = Text[TextIndex];
        }
        
        s32 FontWidth, FontHeight;
        s32 AdvanceWidth, LeftSideBearing;
        s32 X0, Y0, X1, Y1;
        u8 *FontBitmap = 0;
        // TODO(luca): Get rid of malloc.
        FontBitmap = stbtt_GetCodepointBitmap(&Font->Info, 
                                              FontScale, FontScale, 
                                              CharAt, 
                                              &FontWidth, &FontHeight, 0, 0);
        stbtt_GetCodepointBitmapBox(&Font->Info, CharAt, 
                                    FontScale, FontScale, 
                                    &X0, &Y0, &X1, &Y1);
        r32 YOffset = Offset.Y + Y0;
        stbtt_GetCodepointHMetrics(&Font->Info, CharAt, &AdvanceWidth, &LeftSideBearing);
        r32 XOffset = Offset.X + LeftSideBearing*FontScale;
        
        DrawCharacter(Buffer, FontBitmap, FontWidth, FontHeight, XOffset, YOffset, Color);
        
        Offset.X += (AdvanceWidth*FontScale);
        free(FontBitmap);
    }
}

internal void
DrawTextWithAlternatingFonts(game_font *Font1, game_font *Font2,
                             game_offscreen_buffer *Buffer, 
                             r32 FontScale,
                             rune *Text, u32 TextLen, v2 Offset, color_rgb Color)
{
    b32 FontToggle = false;
    for(u32 TextIndex = 0;
        TextIndex < TextLen;
        TextIndex++)
    {
        FontToggle = !FontToggle;
        game_font *Font = (FontToggle) ? Font1 : Font2;
        Assert(Font->Initialized);
        
        rune CharAt = Text[TextIndex];
        
        s32 FontWidth, FontHeight;
        s32 AdvanceWidth, LeftSideBearing;
        s32 X0, Y0, X1, Y1;
        u8 *FontBitmap = 0;
        // TODO(luca): Get rid of malloc.
        FontBitmap = stbtt_GetCodepointBitmap(&Font->Info, 
                                              FontScale, FontScale, 
                                              CharAt, 
                                              &FontWidth, &FontHeight, 0, 0);
        stbtt_GetCodepointBitmapBox(&Font->Info, CharAt, 
                                    FontScale, FontScale, 
                                    &X0, &Y0, &X1, &Y1);
        r32 YOffset = Offset.Y + Y0;
        stbtt_GetCodepointHMetrics(&Font->Info, CharAt, &AdvanceWidth, &LeftSideBearing);
        r32 XOffset = Offset.X + LeftSideBearing*FontScale;
        
        DrawCharacter(Buffer, FontBitmap, FontWidth, FontHeight, XOffset, YOffset, Color);
        
        Offset.X += (AdvanceWidth*FontScale);
        free(FontBitmap);
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

//- Wordled 
internal b32
ValidLetterCountInGuess(rune *Word, u8 *Guess, rune Letter)
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

//- Curl 
internal psize 
CurlCBLog(void *Buffer, psize ItemsSize, psize Size, void *UserPointer)
{
    psize Result = 0;
    if(Size == 1)
    {
        Result = ItemsSize;
    }
    else
    {
        Result = Size;
    }
    
    Log((char *)Buffer);
    return Result;
}

internal psize 
CurlCBWriteToArena(void *Buffer, psize Size, psize DataSize, void *UserPointer)
{
    Assert(Size == 1);
    psize Result = DataSize;
    
    memory_arena *Arena = (memory_arena *)UserPointer;
    
    MemoryCopy((Arena->Base + Arena->Used), Buffer, DataSize);
    Assert(Arena->Used + DataSize <= Arena->Size);
    Arena->Used += DataSize;
    
    return Result;
}

struct get_todays_wordle_curl_params
{
    b32 *Done;
    memory_arena *Arena;
    rune *Word;
};

PLATFORM_WORK_QUEUE_CALLBACK(GetTodaysWordleCurl)
{
    memory_arena *Arena = ((get_todays_wordle_curl_params *)Data)->Arena;
    b32 *Done = ((get_todays_wordle_curl_params *)Data)->Done;
    rune *Word = ((get_todays_wordle_curl_params *)Data)->Word;
    
    char URL[] = "https://www.nytimes.com/svc/wordle/v2";
    
    time_t Now = 0;
    time(&Now);
    struct tm *LocalTimeNow = localtime(&Now);
    
    u8 URLBuffer[256] = {};
    stbsp_sprintf((char *)URLBuffer, "%s/%d-%02d-%d.json", URL,
                  LocalTimeNow->tm_year + 1900,
                  LocalTimeNow->tm_mon + 1,
                  LocalTimeNow->tm_mday);
    
    psize StartPos = Arena->Used;
    
    CURL *CurlHandle = curl_easy_init();
    
    // NOTE(luca): init to NULL is important
    struct curl_slist *Headers = 0;
    
    // NOTE(luca): Cosplay as my computer.
    Headers = curl_slist_append(Headers, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:134.0) Gecko/20100101 Firefox/134.0");
#if 0        
    curl_easy_setopt(CurlHandle, CURLOPT_HEADERFUNCTION, CurlCBLog);
#endif
    
    if(CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_VERBOSE, 0L)) &&
       CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_URL, URLBuffer)) &&
       CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_WRITEFUNCTION, CurlCBWriteToArena)) &&
       CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_WRITEDATA, Arena)) &&
       CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_FOLLOWLOCATION, 1L)) &&
       CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_HTTPHEADER, Headers)) &&
       CURLE_OK == (curl_easy_perform(CurlHandle)))
    {
        char *Str = (char *)(Arena->Base  + StartPos);
        psize Size = (Arena->Used - StartPos);
        
        char MatchStr[] = "\"solution\":";
        b32 IsMatch = false;
        psize MatchAt = 0;
        for(psize ScanIndex = 0;
            ((ScanIndex < Size) && (!IsMatch));
            ScanIndex++)
        {
            MatchAt = ScanIndex;
            IsMatch = true;
            for(psize MatchIndex = 0;
                ((MatchIndex < (sizeof(MatchStr) - 1)) &&
                 (MatchIndex + ScanIndex < Size));
                MatchIndex++)
            {
                if(MatchStr[MatchIndex] != Str[ScanIndex + MatchIndex])
                {
                    IsMatch = false;
                    break;
                }
            }
        }
        
        if(IsMatch)
        {
            MatchAt += (sizeof("\"solution\":\"") - 1);
            for(psize CharIndex = 0;
                CharIndex < WORDLE_LENGTH;
                CharIndex++)
            {
                Word[CharIndex] = Str[MatchAt + CharIndex];
            }
        }
        
    }
    else
    {
        Log("Failed to get HTML.");
    }
    
    curl_easy_cleanup(CurlHandle);
    
    *Done = true;
}

//- Input 
internal void 
AppendCharToInputText(game_state *GameState, rune Codepoint)
{
    if(GameState->TextInputCount < ArrayCount(GameState->TextInputText))
    {
        GameState->TextInputText[GameState->TextInputCount++] = Codepoint;
    }
}

//- Font 
internal void
InitFont(thread_context *Thread, game_font *Font, game_memory *Memory, char *FilePath)
{
    debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Thread, FilePath);
    
    if(File.Contents)
    {
        if(stbtt_InitFont(&Font->Info, (u8 *)File.Contents, stbtt_GetFontOffsetForIndex((u8 *)File.Contents, 0)))
        {
            Font->Info.data = (u8 *)File.Contents;
            
            s32 X0, Y0, X1, Y1;
            stbtt_GetFontBoundingBox(&Font->Info, &X0, &Y0, &X1, &Y1);
            Font->BoundingBox[0] = v2{(r32)X0, (r32)Y0};
            Font->BoundingBox[1] = v2{(r32)X1, (r32)Y1};
            stbtt_GetFontVMetrics(&Font->Info, &Font->Ascent, &Font->Descent, &Font->LineGap);
            Font->Initialized = true;
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

//- Game callbacks 
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    
    if(!Memory->IsInitialized)
    {
        InitFont(Thread, &GameState->RegularFont, Memory, "../data/fonts/jetbrains_mono_regular.ttf");
        InitFont(Thread, &GameState->ItalicFont, Memory, "../data/fonts/jetbrains_mono_italic.ttf");
        InitFont(Thread, &GameState->BoldFont, Memory, "../data/fonts/jetbrains_mono_bold.ttf");
        
        GameState->SelectedColor = SquareColor_Yellow;
        GameState->ExportedPatternIndex = 0;
        
        InitializeArena(&GameState->ScratchArena, Megabytes(1), Memory->TransientStorage);
        GameState->WordleWordIsValid = false;
        
        GameState->TextInputCount = 0;
        GameState->TextInputMode = true;
        
        Log = Memory->PlatformLog;
        curl_global_init(CURL_GLOBAL_ALL);
        
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
                Assert(Controller->Text.Count < ArrayCount(Controller->Text.Buffer));
                
                // TODO(luca): This should use the minimum in case where there are more keys in the Keyboard.TextInputBuffer than in GameState->TextInputText
                for(u32 InputIndex = 0;
                    InputIndex < Controller->Text.Count;
                    InputIndex++)
                {
                    game_text_button Button = Controller->Text.Buffer[InputIndex];
                    
                    // TODO(luca): Check that Codepoint is not shrunk because that would cause inequal value to still be equal if the shrunk part is equal. 
                    
                    u32 *InputCount = &GameState->TextInputCount;
                    
                    if(0) {}
                    else if(Button.Codepoint == 'u' && Button.Control)
                    {
                        *InputCount = 0;
                    }
                    else if(Button.Codepoint == 'h' && Button.Control)
                    {
                        if(*InputCount)
                        {
                            *InputCount -= 1;
                        }
                    }
                    else if((Button.Codepoint == 'w' && Button.Control) ||
                            (Button.Codepoint == '\b' && Button.Control))
                    {
                        // Delete a word.
                        while(*InputCount && GameState->TextInputText[*InputCount - 1] == ' ') *InputCount -= 1;
                        while(*InputCount && GameState->TextInputText[*InputCount - 1] != ' ') *InputCount -= 1;
                    }
                    else if(Button.Control || Button.Alt)
                    {
                        // These can only be shortcuts so since they aren't implemented these will be skipped.
                    }
                    else if(Button.Codepoint == '\b')
                    {
                        if(*InputCount)
                        {
                            *InputCount -= 1;
                        }
                    }
                    else if(Button.Codepoint == '\n' ||
                            Button.Codepoint == '\r')
                    {
                        for(u32 InputIndex = 0;
                            InputIndex < ArrayCount(GameState->WordleWord);
                            InputIndex++)
                        {
                            if(InputIndex < GameState->TextInputCount)
                            {
                                GameState->WordleWord[InputIndex] = (GameState->TextInputText[InputIndex]);
                            }
                            else
                            {
                                GameState->WordleWord[InputIndex] = ' ';
                            }
                        }
                        
                        GameState->WordleWordIsValid = true;
                    }
                    else
                    {
                        // Only allow characters the Wordle game would allow.
                        
                        if((Button.Codepoint >= 'A' &&
                            (Button.Codepoint) <= 'Z'))
                        {
                            Button.Codepoint = (Button.Codepoint - 'A') + 'a';
                        }
                        
                        if((Button.Codepoint >= 'a' && Button.Codepoint <= 'z') || Button.Codepoint == ' ')
                        {
                            AppendCharToInputText(GameState, Button.Codepoint);
                        }
                    }
                }
                
                if(WasPressed(Input->MouseButtons[PlatformMouseButton_ScrollUp]))
                {
                    u32 ColorIndex = GameState->SelectedColor;
                    if(ColorIndex) ColorIndex--;
                    GameState->SelectedColor = ColorIndex;
                }
                if(WasPressed(Input->MouseButtons[PlatformMouseButton_ScrollDown]))
                {
                    u32 ColorIndex = GameState->SelectedColor;
                    if(ColorIndex + 1 < SquareColor_Count) ColorIndex++;
                    GameState->SelectedColor = ColorIndex;
                }
                
                if(WasPressed(Controller->ActionLeft))
                {
                    GameState->TextInputMode = !GameState->TextInputMode;
                }
                
                if(WasPressed(Controller->ActionUp))
                {
                    GameState->WordleWordIsValid = false;
                    Memory->PlatformAddEntry(Memory->HighPriorityQueue, GetTodaysWordleCurl, &(get_todays_wordle_curl_params){
                                                 .Done = &GameState->WordleWordIsValid,
                                                 .Arena = &GameState->ScratchArena,
                                                 .Word = GameState->WordleWord, 
                                             });
                }
                
            }
        }
    }
    Assert(GameState->SelectedColor < SquareColor_Count);
    
    // Draw wordle rectangles
    r32 WordleRectWidth = 48.0f;
    s32 WordleRectsRows = 6;
    s32 WordleRectsColumns = 5;
    v2 WordleRectsBase = 0.5f*v2{
        Buffer->Width - WordleRectsColumns*WordleRectWidth, 
        Buffer->Height - WordleRectsRows*WordleRectWidth
    };
    v2 WordleRectsPadding = {2.0f, 2.0f};
    {
        r32 Width = WordleRectWidth;
        v2 Base = WordleRectsBase;
        s32 Rows = WordleRectsRows;
        s32 Columns = WordleRectsColumns;
        
        v2 Padding = WordleRectsPadding;
        
        s32 SelectedX = CeilReal32ToInt32((r32)(Input->MouseX - Base.X)/(r32)(Width + Padding.X)) - 1;
        s32 SelectedY = CeilReal32ToInt32((r32)(Input->MouseY - Base.Y)/(r32)(Width + Padding.Y)) - 1;
        
        v2 vMin = {};
        v2 vMax = {};
        
        vMin = Base;
        for(s32 Y = 0;
            Y < Rows;
            Y++)
        {
            for(s32 X = 0;
                X < Columns;
                X++)
            {
                color_rgb Color = {1.0f, 1.0f, 0.0f};
                
                vMax = vMin + Width;
                
                if((X == SelectedX) &&
                   (Y == SelectedY))
                {
                    if(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                    {
                        GameState->PatternGrid[Y][X] = GameState->SelectedColor;
                    }
                    Color = GetColorRGBForColorIndex(GameState->SelectedColor);
                    DrawRectangle(Buffer, vMin, vMax, color_rgb(0.0f)); // Change to white
                    DrawRectangle(Buffer, 
                                  vMin + Padding, vMax - Padding,
                                  Color);
                }
                else
                {
                    u32 PatternValue = GameState->PatternGrid[Y][X];
                    Color = GetColorRGBForColorIndex(PatternValue);
                    DrawRectangle(Buffer, vMin, vMax, Color);
                }
                
                
                vMin.X += Padding.X + Width;
            }
            vMin.X = Base.X;
            vMin.Y += Padding.Y + Width;
        }
    }
    
    // Selected color wheel
    {
        v2 Padding = WordleRectsPadding;
        v2 SelectedBorder = WordleRectsPadding*1;
        v2 Base = WordleRectsBase;
        r32 Width = 0.5f*WordleRectWidth;
        color_rgb BorderColor = color_rgb(0.7f);
        color_rgb SelectedBorderColor = color_rgb(0.7f);
        
        Base.X -= (Width + Padding.X + SelectedBorder.X)+ Width ;
        Base.Y += SelectedBorder.Y;
        
        // Draw Background
        {
            v2 vMin = Base - Padding;
            v2 vMax = Base + Width + Padding;
            vMax.Y += (Width + Padding.Y + SelectedBorder.Y)*(SquareColor_Count - 1);
            DrawRectangle(Buffer, vMin, vMax, color_rgb(0.2f));
        }
        
        // Draw color rectangle
        {        
            v2 vMin = Base;
            for(s32 ColorIndex = 0;
                ColorIndex < SquareColor_Count;
                ColorIndex++)
            {
                color_rgb Color = GetColorRGBForColorIndex(ColorIndex);
                
                
                if(GameState->SelectedColor == ColorIndex)
                {
                    DrawRectangle(Buffer, vMin, vMin + Width, 1.2f*Color);
                }
                
                DrawRectangle(Buffer, vMin + SelectedBorder, vMin + Width - SelectedBorder, Color);
                
                vMin.Y += Width + Padding.Y + SelectedBorder.X;
            }
        }
        
    }
    
    char Text[256] = {};
    int TextLen = 0;
    r32 FontScale = 0.0f;
    r32 YAdvance = 0.0f;
    r32 Baseline = 0.0f;
    v2 TextOffset = {};
    int AdvanceWidth = 0;
    
    game_font DefaultFont = GameState->RegularFont;
    
    // Prepare drawing of the guesses.
    FontScale = stbtt_ScaleForPixelHeight(&DefaultFont.Info, 24.0f);
    YAdvance = FontScale*(DefaultFont.Ascent - 
                          DefaultFont.Descent + 
                          DefaultFont.LineGap);
    Baseline = (DefaultFont.Ascent*FontScale);
    
    TextOffset = v2{16.0f, 16.0f + Baseline};
    
    if(GameState->WordleWordIsValid)
    {
        DrawText(Buffer, &DefaultFont, FontScale, 
                 ArrayCount(GameState->WordleWord), (u8 *)GameState->WordleWord,
                 TextOffset + -v2{8.0f, 0.0f}, color_rgb(1.0f), true);
        
        TextOffset.Y += YAdvance*2.0f;
        
        //-Matche the pattern
        rune *Word = GameState->WordleWord;
        debug_read_file_result WordsFile = Memory->DEBUGPlatformReadEntireFile(Thread, "../data/words.txt");
        
        int WordsCount = WordsFile.ContentsSize / WORDLE_LENGTH;
        if(WordsFile.Contents)
        {
            u8 *Words = (u8 *)WordsFile.Contents;
            
            s32 PatternRowAt = 0;
            s32 PatternRowsCount = 6;
            
            for(s32 WordsIndex = 0;
                ((WordsIndex < WordsCount) && 
                 (PatternRowAt < PatternRowsCount));
                WordsIndex++)
            {
                // Match the pattern's row against the guess.
                // TODO(luca): Check if the guess == the word and skip it otherwise it would end the game.
                s32 PatternMatches = 1;
                u8 *Guess = &Words[WordsIndex*(WORDLE_LENGTH+1)];
                for(s32 CharIndex = 0;
                    ((CharIndex < WORDLE_LENGTH) &&
                     (PatternMatches));
                    CharIndex++)
                {
                    u8 GuessCh = Guess[CharIndex];
                    s32 PatternValue = GameState->PatternGrid[PatternRowAt][CharIndex];
                    
                    if(PatternValue == SquareColor_Green)
                    {
                        PatternMatches = (GuessCh == Word[CharIndex]);
                    }
                    else if(PatternValue == SquareColor_Yellow)
                    {
                        PatternMatches = 0;
                        for(s32 CharAt = 0;
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
                        for(s32 CharAt = 0;
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
                        for(s32 CharAt = 0;
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
                    DrawText(Buffer, &DefaultFont, FontScale,
                             WORDLE_LENGTH, Guess, 
                             TextOffset, color_rgb(1.0f), false);
                    
                    TextOffset.Y += YAdvance;
                    
                    WordsIndex = 0;
                    PatternRowAt++;
                }
            }
        }
        
        Memory->DEBUGPlatformFreeFileMemory(Thread, WordsFile.Contents, WordsFile.ContentsSize);
    }
    
    // NOTE(luca): Debug code for drawing inputted text.
#if 1
    {
        r32 FontScale = stbtt_ScaleForPixelHeight(&DefaultFont.Info, 20.0f);
        
        v2 Offset = {100.0f, 30.0f};
        
        r32 TextHeight = FontScale*(DefaultFont.Ascent - 
                                    DefaultFont.Descent + 
                                    DefaultFont.LineGap);
        r32 TextWidth = 0;
        for(u32 InputIndex = 0;
            InputIndex < GameState->TextInputCount;
            InputIndex++)
        {
            s32 AdvanceWidth, LeftSideBearing;
            rune Codepoint = GameState->TextInputText[InputIndex];
            stbtt_GetCodepointHMetrics(&DefaultFont.Info, Codepoint,
                                       &AdvanceWidth, &LeftSideBearing);
            TextWidth += (AdvanceWidth)*FontScale;
        }
        
        r32 Baseline = (DefaultFont.Ascent*FontScale);
        
        color_rgb ColorBorder = color_rgb(1.0f);
        color_rgb ColorBG = {0.007f, 0.007f, 0.007f};
        color_rgb ColorFG = {0.682f, 0.545f, 0.384f};
        
        r32 CursorWidth = 7.0f;
        r32 CursorHeight = TextHeight;
        
        // Draw text box
        {        
            v2 vMin = {Offset.X, Offset.Y - Baseline};
            v2 vMax = {Offset.X + TextWidth + CursorWidth, vMin.Y + TextHeight};
            DrawRectangle(Buffer, vMin + -2, vMax + 2, ColorBorder);
            DrawRectangle(Buffer, vMin + -1, vMax + 1, ColorBG);
        }
        
        // Draw cursor
        {
            v2 vMin = {Offset.X + TextWidth, Offset.Y - Baseline};
            v2 vMax = {vMin.X + CursorWidth, vMin.Y + TextHeight};
            DrawRectangle(Buffer, vMin, vMax, ColorFG);
        }
        
        // Draw the text
        DrawText(Buffer, &DefaultFont, FontScale,
                 GameState->TextInputCount, (u8 *)GameState->TextInputText,  
                 Offset, ColorFG, true);
        
        Assert(GameState->TextInputCount <= ArrayCount(GameState->TextInputText));
    }
#endif
    
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer);
}
