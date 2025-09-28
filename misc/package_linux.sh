#!/bin/sh

ThisDir="$(dirname "$(readlink -f "$0")")"
cd "$ThisDir"/..

mkdir -p build
./code/build.sh release

Zip="./build/wordled.zip"
>&2 printf 'INFO: Creating zip.\n'
rm -f "$Zip"
zip "$Zip" build/linux_handmade build/handmade.so 
zip -r "$Zip" data/fonts data/words.txt

Tar="./build/wordled.tar.gz"
rm -f "$Tar"
>&2 printf 'INFO: Creating tar.\n'
tar --dereference --create --verbose --gzip --file "$Tar" build/linux_handmade build/handmade.so data/fonts/ data/words.txt
