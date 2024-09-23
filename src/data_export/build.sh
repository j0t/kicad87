#!/bin/sh

AS=as
GCC=x86_64-w64-mingw32-c++
DLLTOOL=dlltool
CV2PDB64=/c/toolz/cv2pdb64

# LINK_OPTS=-s
# -DDEBUG
CC_OPTS='-ggdb -O0 -DUNICODE -D_WIN32_WINNT=0x0602 -DDEBUG'

# this is just our Drosophila

# define its exports 

cat << EOF > util.def
LIBRARY util
EXPORTS
??_7FOOTPRINT_INFO@@6B@=Table_133 DATA
??_7FOOTPRINT_LIST@@6B@=Table_134 DATA
EOF

# build util.dll
${GCC} ${CC_OPTS} -c -o util.o util.c
${GCC} ${LINK_OPTS} -o util.dll -shared util.o util.def -Wl,--subsystem,windows

# define exports for fwd.dll
# export_name=c_symbol @# DATA

cat << EOF > fwd_exp.def
LIBRARY fwd_lib
EXPORTS
??_7FOOTPRINT_INFO@@6B@=Table_133 @3 DATA
??_7FOOTPRINT_LIST@@6B@=Table_134 @2 DATA
LoadUtil @1
EOF

# build wrapper library
${GCC} ${CC_OPTS} -c -o fwd.o fwd.c
${GCC} ${LINK_OPTS} -o fwd.dll -shared fwd.o fwd_exp.def -Wl,--subsystem,windows -lntdll
${CV2PDB64} fwd.dll

# build test.exe
${GCC} ${CC_OPTS} -c -o test.o test.c
${GCC} ${LINK_OPTS} -o t.exe test.o

${CV2PDB64} t.exe
