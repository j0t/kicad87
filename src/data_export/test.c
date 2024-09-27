#include <stdio.h>
#include <windows.h>
#include <ntdef.h>

typedef void (*fptr)();

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
    
    fptr * pTable = (fptr *)GetProcAddress(hModule, "??_7FOOTPRINT_INFO@@6B@");
    if( !pTable )
    {
        fprintf(stderr, "Couldn't get fwd.??_7FOOTPRINT_INFO@@6B@ symbol\n");
    }

    FARPROC pCallFoo = GetProcAddress(hModule, "CallFoo"); // it should trigger util.dll loading via deferred import
    if( pCallFoo )
        pCallFoo();
    else
        fprintf(stderr, "Couldn't get fwd.CallFoo symbol\n");
/*    
    FARPROC LoadUtil = GetProcAddress(hModule, "LoadUtil");
    if( !LoadUtil )
    {
        fprintf(stderr, "Couldn't get fwd.LoadUtil symbol\n");
    }
    else
    {
        LoadUtil();
    }
*/    
    if( pTable[0] )
        pTable[0]();
    else
        fprintf(stderr, "pTable[0] == NULL\n");
    
    if( pTable[1] )
        pTable[1]();
    else
        fprintf(stderr, "pTable[1] == NULL\n");
#endif
#if !defined( USE_IMPORT_LIB )
    FreeLibrary( hModule );
#endif
    return 0;
}
