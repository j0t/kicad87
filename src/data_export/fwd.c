#include <stdio.h>
#include <windows.h>
#include <ntdef.h>

#include "ldr_dll.h"

#if defined( DEBUG )
#define FRRINTF fprintf
#else
#define FRRINTF
#endif

typedef void (*fptr)();

__declspec(dllexport) fptr Table[2];

extern "C" __declspec(dllexport) void LoadUtil()
{
    HMODULE hModule = LoadLibraryW(L"util.dll");
    if( !hModule )
    {
        FRRINTF(stderr, "fwd.setup: Couldn't load util.dll\n");
        return;
    }
}

static HMODULE DLLBase = 0;

static VOID CALLBACK OnLoadHook( ULONG NotificationReason,
                       PLDR_DLL_NOTIFICATION_DATA NotificationData,
                       PVOID Context )
{
    PCUNICODE_STRING BaseDllName = NotificationData->Loaded.BaseDllName;
    
    FRRINTF(stderr, "fwd.OnLoadHook: %Z NotificationReason %d\n", BaseDllName, NotificationReason );
    
    if( NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED )
        return;

    if( !_wcsnicmp( L"util.DLL", BaseDllName->Buffer, BaseDllName->Length/sizeof(WCHAR)) )
    {
         DLLBase = (HMODULE)NotificationData->Loaded.DllBase;        
         
         fptr * pTable = (fptr *)GetProcAddress(DLLBase, "Table");
         if( !pTable )
         {
              FRRINTF(stderr, "fwd.setup Couldn't get util.Table symbol\n");
         }
         else
         {
              memmove( Table, (const void*)pTable, sizeof(fptr)*2 );
         }
    }
}

static HANDLE g_Cookie;

static void RegisterHook()
{
    LdrRegisterDllNotification( 0, &OnLoadHook, NULL, &g_Cookie );
}

static void UnregisterHook()
{
    LdrUnregisterDllNotification( g_Cookie );
    g_Cookie = NULL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    fprintf(stderr,"fwd.DllMain: %x %d\n", hinstDLL, fdwReason);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
             RegisterHook();
             break;
             
        case DLL_PROCESS_DETACH:
             UnregisterHook();
             break;
    }
    return TRUE;
}