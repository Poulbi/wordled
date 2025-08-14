#!/bin/sh

ThisDir="$(dirname "$(readlink -f "$0")")"
cd "$ThisDir"

mkdir ../build > /dev/null 2>&1

# Supported: clang, g++
Compiler="clang"

CompilerFlags="
-O0
-ggdb
-DHANDMADE_INTERNAL
-DHANDMADE_SLOW
-DOS_LINUX
-nostdinc++
"

WarningFlags="-Wall
-Wextra
-Wno-unused-but-set-variable
-Wno-unused-variable
-Wno-write-strings
-Wno-pointer-arith
-Wno-unused-parameter
-Wno-unused-function"

ClangCompilerFlags="
-ftime-trace
"
ClangWarningFlags="
-Wno-null-dereference
-Wno-missing-braces
-Wno-vla-cxx-extension
-Wno-writable-strings
"

# Platform specific linker flags
LinuxLinkerFlags="
-lpthread
-lasound
-lm
-lX11
-lXfixes"

if [ "$Compiler" = "clang" ]
then
 CompilerFlags="$CompilerFlags $ClangCompilerFlags"
 WarningFlags="$WarningFlags $ClangWarningFlags"
fi

printf 'linux_handmade.cpp\n'
$Compiler \
 $CompilerFlags \
 $WarningFlags \
 -o ../build/linux_handmade \
 $LinuxLinkerFlags \
 libs/linuxhmh/linux_handmade.cpp

printf 'handmade.cpp\n'
$Compiler \
 $CompilerFlags \
 $WarningFlags \
 -shared \
 -o ../build/handmade.so \
 handmade.cpp
