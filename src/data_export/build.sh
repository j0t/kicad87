#!/bin/sh

AS=as
GCC=x86_64-w64-mingw32-c++
DLLTOOL=dlltool
CV2PDB64=/c/toolz/cv2pdb64

# LINK_OPTS=-s
# -DDEBUG
CC_OPTS='-ggdb -O0 -DUNICODE -D_WIN32_WINNT=0x0602 -DDEBUG'

# build test.dll
${GCC} ${CC_OPTS} -c -o util.o util.c
#${GCC} ${LINK_OPTS} -o util.dll -shared util.o -Wl,--subsystem,windows,--out-implib,libutil.a
${GCC} ${LINK_OPTS} -o util.dll -shared util.o -Wl,--subsystem,windows

cat << EOF > util.def
LIBRARY util
EXPORTS
Table DATA
EOF

IMPLIB_OPT=--output-lib

# we are using delayed load
IMPLIB_OPT=--output-delaylib

${DLLTOOL} --input-def util.def ${IMPLIB_OPT} libutil.a

ar -x libutil.a

# build wrapper library
#${GCC} ${CC_OPTS} -c -o fwd_util.o fwd_util.c
#${GCC} ${LINK_OPTS} -o fwd_test.dll -shared fwd_util.o wraps.o -Wl,--subsystem,windows,--out-implib,libfwd_test.a -L. -lntdll -ltest

as w.s -o w.o

${GCC} ${CC_OPTS} -c -o fwd.o fwd.c
${GCC} ${LINK_OPTS} -o fwd.dll -shared fwd.o -Wl,--subsystem,windows -lntdll
${CV2PDB64} fwd.dll

# build test.exe
#${GCC} ${CC_OPTS} -DUSE_IMPORT_LIB -c -o test.o test.c
${GCC} ${CC_OPTS} -c -o test.o test.c

#${GCC} ${LINK_OPTS} -o t.exe test.o w.o
#${GCC} ${LINK_OPTS} -o t.exe test.o libutil_a_t.o libutil_a_h.o libutil_a_s00000.o
${GCC} ${LINK_OPTS} -o t.exe test.o w.o -L. -lutil

${CV2PDB64} t.exe

: '

# Exploring static linking

1. delaylib is not suitable for importing DATA symbols

2. dlltool can build libutil.a from util.def

2.1 list archive `ar -t libutil.a`

libutil_a_t.o      
libutil_a_h.o      
libutil_a_s00000.o 

2.2 extract objects `ar -x libutil.a`

2.3 `objdump -h `ls lib*.o`

libutil_a_h.o:     file format pe-x86-64

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  3 .idata$2      00000014  0000000000000000  0000000000000000  00000104  2**2
                  CONTENTS, ALLOC, LOAD, RELOC, DATA

libutil_a_s00000.o:     file format pe-x86-64

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  3 .idata$7      00000004  0000000000000000  0000000000000000  0000012c  2**2
                  CONTENTS, RELOC
  4 .idata$5      00000008  0000000000000000  0000000000000000  00000130  2**2
                  CONTENTS, RELOC
  5 .idata$4      00000008  0000000000000000  0000000000000000  00000138  2**2
                  CONTENTS, RELOC
  6 .idata$6      00000008  0000000000000000  0000000000000000  00000140  2**1
                  CONTENTS

libutil_a_t.o:     file format pe-x86-64

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  3 .idata$4      00000008  0000000000000000  0000000000000000  00000104  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  4 .idata$5      00000008  0000000000000000  0000000000000000  0000010c  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  5 .idata$7      0000000c  0000000000000000  0000000000000000  00000114  2**2
                  CONTENTS, ALLOC, LOAD, DATA

3. DATA symbols can be requested by GetProcAddress

'
