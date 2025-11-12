#!/bin/sh

set -eu

ScriptDirectory="$(dirname "$(readlink -f "$0")")"
cd "$ScriptDirectory"

mkdir -p ../build
Output="../build/build"

if [ ! -x "$Output" ] 
then
 cc -Wno-write-strings -g -o "$Output" build.cpp
fi

$Output
