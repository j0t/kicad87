#!/bin/sh

# input 
SRC_NAME=__eeschema
WRP_NAME=_eeschema
SRC_DLL=/d/kicad.raw/bin/${SRC_NAME}.dll
DATA_LST=../data.lst

# output
DEF1=${SRC_NAME}_fun.def
DEF2=${SRC_NAME}_dat.def
ENTRIES_H=entries.h

# tools
AS=as
GCC=x86_64-w64-mingw32-c++
DLLTOOL=dlltool
CV2PDB64=/c/toolz/cv2pdb64
GENDEF=gendef

# options
#CC_OPTS='-ggdb -O0 -DUNICODE -D_WIN32_WINNT=0x0602 -DDEBUG'
CC_OPTS='-O3 -DUNICODE -D_WIN32_WINNT=0x0602'

# generate .def files

# export FUNCTIONS, forwarded to delayed import

echo "LIBRARY ${SRC_NAME}" > ${DEF1}
echo 'EXPORTS' >> ${DEF1}
# add function exports
#${GENDEF} - ${SRC_DLL} 2>/dev/null | grep -v '^;' | tail -n +3 | cat -n | grep -v 'DATA$' | awk -F " " '{print "_"$2"=="$2" @"$1" "$3}' >> ${DEF1}
#^ DLLTOOL doesn't accepts @ordinals

${GENDEF} - ${SRC_DLL} 2>/dev/null | grep -v '^;' | tail -n +3 | cat -n | grep -v 'DATA$' | awk -F " " '{print "_"$2"=="$2}' >> ${DEF1}

# generate delayed-load import library
${DLLTOOL} -v --input-def ${DEF1} --output-delaylib lib${SRC_NAME}.a

# export DATA (reconstructed)

echo "LIBRARY ${WRP_NAME}" > ${DEF2}
echo 'EXPORTS' >> ${DEF2}
# add DATA exports (vtables)
cat ${DATA_LST} | awk -F " " '{print $2"=Table_"$1" @"$1" DATA"}' >> ${DEF2}

# generate header for DATA
cat ${DATA_LST} | awk -F " " '{print "DD_ENTRY("$1",\""$2"\","$3")"}' > ${ENTRIES_H}

# generate assembly source for function wrappers	
echo -e '\t.include "wrap.inc"' > wraps.s
echo -e '\t.text' >> wraps.s
cat ${DEF1} | sed '1,2d;s/.*==\(.*\)/\tWRAP \1/g' >> wraps.s

# compile function wrappers assembly
${AS} wraps.s -o wraps.o

# build wrapper dll
${GCC} ${CC_OPTS} -DSRC_DLL_NAME=${WRP_NAME} -c -o fwd_util.o fwd_util.c
${GCC} ${LINK_OPTS} -o ${WRP_NAME}.dll -shared fwd_util.o wraps.o ${DEF2} -Wl,--subsystem,windows -lntdll -L. -l${SRC_NAME}

${CV2PDB64} ${WRP_NAME}.dll
