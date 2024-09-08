#!/bin/sh

# input 
SRC_NAME=_eeschema
SRC_DLL=/d/kicad.raw/bin/${SRC_NAME}.dll
DEF=${SRC_NAME}.def

# tools 
AS=as
GCC=x86_64-w64-mingw32-c++
DLLTOOL=dlltool

# generate .def file 
echo 'LIBRARY "_eeschema.dll"' > ${DEF}
echo 'EXPORTS' >> ${DEF}
gendef - ${SRC_DLL} | grep -v '^;' | tail +3 | cut -f1 -d' ' | sed 's/.*/_\0==\0/g' >> ${DEF}

${DLLTOOL} --input-def ${DEF} --output-delaylib lib${SRC_NAME}.a

# generate ASM file for wrappers	
echo -e '\t.include "wrap.inc"' > wraps.s
echo -e '\t.text' >> wraps.s
cat ${DEF} | sed '1,2d;s/.*==\(.*\)/\tWRAP \1/g' >> wraps.s

# compile ASM
${AS} wraps.s -o wraps.o
