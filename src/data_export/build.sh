#!/bin/sh

AS=as
GCC=x86_64-w64-mingw32-c++
DLLTOOL=dlltool

# LINK_OPTS=-s
# -DDEBUG
CC_OPTS='-ggdb -O0 -DUNICODE -D_WIN32_WINNT=0x0602'

# build test.dll
${GCC} ${CC_OPTS} -c -o util.o util.c
${GCC} ${LINK_OPTS} -o util.dll -shared util.o -Wl,--subsystem,windows,--out-implib,libutil.a

#${DLLTOOL} --input-def ${DEF} --output-delaylib lib${SRC_NAME}.a

cat << EOF > util.def
LIBRARY util
EXPORTS
Table DATA
EOF

#${DLLTOOL} --input-def util.def --output-lib libutil.a
${DLLTOOL} --input-def util.def --output-delaylib libutil.a

# build wrapper library
#${GCC} ${CC_OPTS} -c -o fwd_util.o fwd_util.c
#${GCC} ${LINK_OPTS} -o fwd_test.dll -shared fwd_util.o wraps.o -Wl,--subsystem,windows,--out-implib,libfwd_test.a -L. -lntdll -ltest

as w.s -o w.o

# build test.exe
#${GCC} ${CC_OPTS} -DUSE_IMPORT_LIB -c -o test.o test.c
${GCC} ${CC_OPTS} -c -o test.o test.c

${GCC} ${LINK_OPTS} -o t.exe test.o w.o
#${GCC} ${LINK_OPTS} -o t.exe test.o 

#-L. -lutil
