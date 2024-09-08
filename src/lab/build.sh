#!/bin/sh

AS=as
GCC=x86_64-w64-mingw32-c++
DLLTOOL=dlltool

# LINK_OPTS=-s
# -DDEBUG
CC_OPTS='-ggdb -O0 -DUNICODE -D_WIN32_WINNT=0x0602'

# build test.dll
${GCC} ${CC_OPTS} -c -o util.o util.c
${GCC} ${LINK_OPTS} -o test.dll -shared util.o -Wl,--subsystem,windows

gendef - test.dll | sed '8,$s/.*/_\0==\0/g' 1> test.def

# build a special import library with delay option
${DLLTOOL} --input-def test.def --output-delaylib libtest.a

# generate ASM file for wrappers	
echo -e '\t.include "wrap.inc"' > wraps.s
echo -e '\t.text' >> wraps.s
sed '1,7d;s/.*==\(.*\)/\tWRAP \1/g' test.def >> wraps.s

# build wrappers object code
${AS} wraps.s -o wraps.o

# build wrapper library
${GCC} ${CC_OPTS} -c -o fwd_util.o fwd_util.c
${GCC} ${LINK_OPTS} -o fwd_test.dll -shared fwd_util.o wraps.o -Wl,--subsystem,windows,--out-implib,libfwd_test.a -L. -lntdll -ltest

# build test.exe
${GCC} ${CC_OPTS} -DUSE_IMPORT_LIB -c -o test.o test.c
${GCC} ${LINK_OPTS} -o t.exe test.o -L. -lntdll -lfwd_test
