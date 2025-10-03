#!/bin/sh

ThisDir="$(dirname "$(readlink -f "$0")")"
cd "$ThisDir"

mkdir ../build > /dev/null 2>&1

# Supported: clang
Compiler="clang"

CompilerFlags="
-O0
-ggdb
-g3
-DHANDMADE_INTERNAL
-DHANDMADE_SLOW
-DOS_LINUX
-DHANDMADE_SMALL_RESOLUTION
-nostdinc++

"

WarningFlags="-Wall
-Wextra
-Wno-sign-compare
-Wno-unused-but-set-variable
-Wno-unused-variable
-Wno-write-strings
-Wno-pointer-arith
-Wno-unused-parameter
-Wno-unused-function
-Wno-int-to-pointer-cast
-Wno-missing-field-initializers
"

ClangCompilerFlags="
-fdiagnostics-absolute-paths
-ftime-trace
"
ClangWarningFlags="
-Wno-null-dereference
-Wno-missing-braces
-Wno-vla-extension
-Wno-writable-strings
-Wno-address-of-temporary
-Wno-reorder-init-list
"

# Platform specific linker flags
LinuxLinkerFlags="
-lpthread
-lasound
-lcurl
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
 -fPIC \
 -shared \
 -o ../build/handmade.so \
 handmade.cpp
