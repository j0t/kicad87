# An experiments with handling of missing imports/exports in PE file

## How to use/install

1. `mv ${kicad}/bin/_eeschema.dll ${kicad}/bin/__eeschema.dll`
2. build `_eeschema.dll`
3. `cp _eeschema.dll ${kicad}/bin/`

## Build and develop

add MSVS build tools to PATH to make `cv2pdb64` working:

~~~
export PATH=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio\ 14.0/VC/bin/amd64:$PATH
~~~

### extract `export_number	export_symbol	table_size` rows from IDA listing `data.ida`
~~~
grep ';' data.ida | cut -d';' -f2 | paste - - | sed 's/ Exported entry \([[:digit:]]\+\)\./\1\t/;s/void[^[]*\[//;s/\])()//;s/ //g' > data.lst
~~~

### .def file EXPORTS directive format
~~~
EXPORTS 
( ( ( name1 [ = name2 ] ) | ( name1 = module-name . external-name ) ) [ == its_name ] [integer] [NONAME] [CONSTANT] [DATA] [PRIVATE] ) *
~~~
Declares name1 as an exported symbol from the DLL, with optional ordinal number integer, 
or declares name1 as an alias (forward) of the function external-name in the DLL. 
If its_name is specified, this name is used as string in export table. module-name. 
Note: The EXPORTS has to be the last command in .def file, as keywords are treated - beside LIBRARY - as simple name-identifiers.
If you want to use LIBRARY as name then you need to quote it.
