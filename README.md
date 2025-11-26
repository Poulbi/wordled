# Wordled: Impress your friends with Wordle drawings!

## Overview
`Wordled` is a GUI application that lets you draw patterns such that you can later input them in the [Wordle](https://www.nytimes.com/games/wordle/index.html) game.  It will then generate the guesses to achieve the required pattern.
                                                                                                              
## Showcase
<video autoplay loop muted src="https://git.spacehb.net/wordled/plain/showcase.mp4?h=main"></video>

## Run
Both following build steps will output executable programs in the `build` directory.  Do note that
this repository already has prebuilt binaries.
For both Windows and Linux you need a working C++ compiler.

### On Windows
Run the `win32_handmade.exe` executable from the `build` directory.

### On Linux
Since I cannot guarantee binary compability, please follow the build instructions.

Following dependencies are required, but should already be installed on most distributions.

- [hm_linux](https://git.spacehb.net/hm_linux/about/?h=main), my linux platform layer.  Put it in `./code/libs/`.
- `libasound.so`
- `libcurl.so`
- `libX11.so` and `libXFixes.so`

In the `code` directory run.
```bat
./build.sh
```
Afterwards run the `linux_handmade` executable in the `build` directory.
