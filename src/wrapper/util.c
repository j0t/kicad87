#include <windows.h>
#include <stdio.h>

#if defined( DEBUG )
#define FRRINTF fprintf
#else
#define FRRINTF
#endif

extern "C" __declspec(dllexport) HANDLE OpenFileUtil(const wchar_t * file_name)
{
#if _WIN32_WINNT >= 0x0602
    CREATEFILE2_EXTENDED_PARAMETERS extParams = {0};
    extParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
   
	return CreateFile2(file_name, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extParams);
#else
    return CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ,
       			        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif    
}

extern "C" __declspec(dllexport) DWORD ProcessId()
{
    return GetCurrentProcessId();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    FRRINTF(stderr,"util.DllMain: %x %d\n", hinstDLL, fdwReason);
    return TRUE;
}