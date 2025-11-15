@echo off

call cache.bat

set CommonCompilerFlags=-MTd -Gm- -nologo -GR- -EHa- -WX -W4 -wd4459 -wd4456 -wd4201 -wd4100 -wd4189 -wd4505 -wd4996 -wd4389 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -FC -Z7
set CommonLinkerFlags=-opt:ref -incremental:no user32.lib Gdi32.lib winmm.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM 32-Bit
REM cl %CommonCompilerFlags% ..\code\libs\hm_win32\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags% 

REM 64-Bit
cl %CommonCompilerFlags% ..\code\handmade.cpp -Fmhandmade.map -LD /link /DLL /EXPORT:GameGetSoundSamples /EXPORT:GameUpdateAndRender

cl %CommonCompilerFlags% ..\code\libs\hm_win32\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags% 

popd
