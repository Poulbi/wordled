#!/bin/sh

ThisDir="$(dirname "$(readlink -f "$0")")"
cd "$ThisDir"

Zip="game.zip"

cd ..
rm -f "$Zip"
zip "$Zip" build/linux_handmade build/handmade.so 
zip "$Zip" data/font.ttf data/words.txt
