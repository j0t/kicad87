#include <stdio.h>
#include <windows.h>
#include <ntdef.h>

#if defined( USE_IMPORT_LIB )
extern "C" {
__declspec(dllimport) DWORD ProcessId();
__declspec(dllimport) HANDLE OpenFileUtil(const wchar_t * file_name);
}
#else
typedef DWORD (*DWORD_VOID_PTR)();
typedef HANDLE (*OpenFileUtilPtr)(const wchar_t * file_name);

DWORD_VOID_PTR ProcessId;
OpenFileUtilPtr OpenFileUtil;
#endif

int main()
{
    fprintf(stderr, "Starting test\n");
    
#if !defined( USE_IMPORT_LIB )
    HMODULE hModule = LoadLibraryW(L"fwd_test.dll");
    if( !hModule )
    {
        fprintf(stderr, "Couldn't load fwd_test.dll\n");
        return 1;
    }
    
    fprintf(stderr, "GetProcAddress for ProcessId\n");
    ProcessId = (DWORD_VOID_PTR) GetProcAddress(hModule, "ProcessId");
    fprintf(stderr, ProcessId ? "OK\n": "FAIL\n" );
    
    fprintf(stderr, "GetProcAddress for OpenFileUtil\n");
    OpenFileUtil = (OpenFileUtilPtr) GetProcAddress(hModule, "OpenFileUtil");
    fprintf(stderr, OpenFileUtil ? "OK\n": "FAIL\n" );
#endif

    if( ProcessId )
    {
        DWORD pid = ProcessId();
        fprintf(stderr, "DLLPID: %d\n", pid);
    }

    if( OpenFileUtil )
    {
        HANDLE hFile = OpenFileUtil(L"test.txt");
        if( INVALID_HANDLE_VALUE == hFile )
        {
            fprintf(stderr, "Error opening file\n");
            return 1;
        }

        char buffer[256] = {0};
        DWORD read = 0;
        if( !ReadFile( hFile, buffer, sizeof(buffer), &read, NULL ) )
        {
            fprintf(stderr, "Error reading file\n");
            return 1;
        }

        fprintf(stderr, "%d bytes: %s\n", read, buffer);

        CloseHandle( hFile );
    }

#if !defined( USE_IMPORT_LIB )
    FreeLibrary( hModule );
#endif
    return 0;
}
