#include <stdio.h>
#include <windows.h>
#include <ntdef.h>

#include "ldr_dll.h"

#if defined( DEBUG )
#define FRRINTF fprintf
#else
#define FRRINTF
#endif

#define DD_ENTRY1(D,N,S) void * Table_##D[S];
#define BEGIN_DD_ENTRY2 static DataDescriptor descs[] = {
#define DD_ENTRY2(D,N,S) {N,Table_##D,sizeof(Table_##D)},
#define END_DD_ENTRY2  };

struct DataDescriptor
{
    const char * name;
    void      * table;
    unsigned int size;
};

#define DD_ENTRY DD_ENTRY1
#include "entries.h" // DD_ENTRY(133,"??_7FOOTPRINT_INFO@@6B@",2)
BEGIN_DD_ENTRY2
#undef DD_ENTRY
#define DD_ENTRY DD_ENTRY2
#include "entries.h"
END_DD_ENTRY2

/*
extern "C" __declspec(dllexport) void LoadUtil()
{
    HMODULE hModule = LoadLibraryW(L"util.dll");
    if( !hModule )
    {
        FRRINTF(stderr, "fwd.setup: Couldn't load util.dll\n");
        return;
    }
}
*/

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
         
         for( unsigned int i = 0; i < sizeof(descs)/sizeof(descs[0]); ++i )
         {
              FARPROC pTable = GetProcAddress(DLLBase, descs[i].name);
              if( pTable )
              {
                  memmove( descs[i].table, (const void *)pTable, descs[i].size );
              }
              else
              {
                  FRRINTF(stderr, "fwd.setup Couldn't get %s symbol\n", descs[i].name); 
              }
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