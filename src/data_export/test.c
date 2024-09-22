#include <stdio.h>
#include <windows.h>
#include <ntdef.h>

typedef void (*fptr)();


//__declspec(dllimport) fptr Table[]; // working with import library
//__declspec(dllimport) fptr * Table;

extern "C" void setupTable( FARPROC );

int main()
{
    fprintf(stderr, "Starting test\n");

#if !defined( USE_IMPORT_LIB )
    HMODULE hModule = LoadLibraryW(L"fwd.dll");
    if( !hModule )
    {
        fprintf(stderr, "Couldn't load fwd.dll\n");
        return 1;
    }
    
    fptr * pTable = (fptr *)GetProcAddress(hModule, "Table");
    if( !pTable )
    {
        fprintf(stderr, "Couldn't get fwd.Table symbol\n");
    }
    
    FARPROC LoadUtil = GetProcAddress(hModule, "LoadUtil");
    if( !LoadUtil )
    {
        fprintf(stderr, "Couldn't get fwd.LoadUtil symbol\n");
    }
    else
    {
        LoadUtil();
    }
    
    pTable[0]();
    pTable[1]();

#endif
/*
    if( Table[0] ) 
        Table[0]();
  
    if( Table[1] )
        Table[1]();
*/
#if !defined( USE_IMPORT_LIB )
    FreeLibrary( hModule );
#endif
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