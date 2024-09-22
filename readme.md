# An experiments with handling of missing imports/exports in PE file

add MSVS build tools to PATH to make `cv2pdb64` morking

~~~
export PATH=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio\ 14.0/VC/bin/amd64:$PATH
~~~

### extract "export_number	export_symbol	table_size" from IDA listing

grep ';' data.ida | cut -d';' -f2 | paste - - | sed 's/ Exported entry \([[:digit:]]\+\)\./\1\t/;s/void[^[]*\[//;s/\])()//;s/ //g' > data.lst

