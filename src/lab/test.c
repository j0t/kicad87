#include <stdio.h>
#include <windows.h>
#include <ntdef.h>

#if defined( IMPLICIT_LOADING )

extern "C" {

__declspec(dllimport) DWORD ProcessId();
__declspec(dllimport) HANDLE OpenFileUtil(const wchar_t * file_name);

}
#else
typedef DWORD (*DWORD_VOID_PTR)();
typedef HANDLE (*OpenFileUtilPtr)(const wchar_t * file_name);

DWORD_VOID_PTR ProcessId, Unregister;
OpenFileUtilPtr OpenFileUtil;
#endif

int main()
{
    puts("Starting test");
    
#if !defined( IMPLICIT_LOADING )
    HMODULE hModule = LoadLibraryW(L"fwd_test.dll");
    
    if( !hModule )
    {
        puts("Couldn't load fwd_test.dll");
        return 1;
    }
    
    puts("GetProcAddress for ProcessId");
    ProcessId = (DWORD_VOID_PTR) GetProcAddress(hModule, "ProcessId");
    if( ProcessId ) puts("OK"); else puts("FAIL");
    
    puts("GetProcAddress for OpenFileUtil");
    OpenFileUtil = (OpenFileUtilPtr) GetProcAddress(hModule, "OpenFileUtil");
    if( OpenFileUtil ) puts("OK"); else puts("FAIL");
    
#endif
    if( ProcessId )
    {
        DWORD pid = ProcessId();
        printf("DLLPID: %d\n", pid);
    }

    if( OpenFileUtil )
    {
        HANDLE hFile = OpenFileUtil(L"test.txt");
        if( INVALID_HANDLE_VALUE == hFile )
        {
            puts("Error opening file");
            return 1;
        }

        char buffer[256] = {0};
        DWORD read = 0;
        if( !ReadFile( hFile, buffer, sizeof(buffer), &read, NULL ) )
        {
            puts("Error reading file");
            return 1;
        }

        printf("%d bytes: %s\n", read, buffer);

        CloseHandle( hFile );
    }

#if !defined( IMPLICIT_LOADING )
    FreeLibrary( hModule );
#endif
    return 0;
}
