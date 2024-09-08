#!/bin/sh

GCC=x86_64-w64-mingw32-c++
OPTS='-ggdb -O0 -DUNICODE'

# build test.dll
#${GCC} ${OPTS} -c -o util.o util.c
${GCC} ${OPTS} -D_WIN32_WINNT=0x0602 -c -o util.o util.c
${GCC} ${OPTS} -o test.dll -shared util.o -Wl,--subsystem,windows

# 
dlltool --input-def test.def --output-delaylib test.lib --dllname test.dll

# build fwd_test.dll
#${GCC} ${OPTS} -c -o fwds.o fwds.c
${GCC} ${OPTS} -D_WIN32_WINNT=0x0602 -c -o fwd_util.o fwd_util.c
${GCC} ${OPTS} -o fwd_test.dll -shared fwd_util.o -Wl,--subsystem,windows,--out-implib,libfwd_test.a -L. -lntdll -ltest

# build test.exe
${GCC} ${OPTS} -D_WIN32_WINNT=0x0602 -c -o test.o test.c
${GCC} ${OPTS} -o t.exe test.o -L. -lntdll

# -lfwd_test 
