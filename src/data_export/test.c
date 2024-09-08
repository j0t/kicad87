#include <stdio.h>
#include <windows.h>
#include <ntdef.h>

typedef void (*fptr)();

//#if !defined( USE_IMPORT_LIB )
//fptr *Table;
//#else
//__declspec(dllimport) fptr Table[2]; // working with import library
__declspec(dllimport) fptr Table[]; // working with import library
//__declspec(dllimport) fptr * Table; // do not working with import library
//#endif

extern "C" void setup();

int main()
{
    fprintf(stderr, "Starting test\n");

//#if !defined( USE_IMPORT_LIB )    
    HMODULE hModule = LoadLibraryW(L"util.dll");
    if( !hModule )
   {
        fprintf(stderr, "Couldn't load util.dll\n");
        return 1;
    }
//    fprintf(stderr, "GetProcAddress for Table\n");
    //GetProcAddress(hModule, "Table");
    //setup();
//    fprintf(stderr, Table ? "OK\n": "FAIL\n" );
//#endif

    if( Table[0] ) 
        Table[0]();
  
    if( Table[1] )
        Table[1]();
    
//#if !defined( USE_IMPORT_LIB )
    //FreeLibrary( hModule );
//#endif  
    return 0;
}
/*
--- WORKING
fptr *Table;

.text:00000001400015FE                 call    rax ; __imp_GetProcAddress
.text:0000000140001600                 mov     cs:Table, rax

.text:000000014000166A                 mov     rax, cs:Table
.text:0000000140001671                 add     rax, 8
.text:0000000140001675                 mov     rax, [rax]
.text:0000000140001678                 call    rax

-- NOT WORKING
__declspec(dllimport) fptr * Table;

.text:00000001400015FE                 call    rax ; __imp_GetProcAddress
.text:0000000140001600                 mov     rdx, cs:__imp_Table
.text:0000000140001607                 mov     [rdx], rax

.text:0000000140001679                 mov     rax, cs:__imp_Table
.text:0000000140001680                 mov     rax, [rax]
.text:0000000140001683                 add     rax, 8
.text:0000000140001687                 mov     rax, [rax]
.text:000000014000168A                 call    rax

*/