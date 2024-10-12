#include <stdio.h>
#include <windows.h>
#include <ntdef.h>
#include <delayimp.h>

#if defined( DEBUG )
#define FRRINTF fprintf
#else
#define FRRINTF
#endif

#include "ldr_dll.h"

#if ! defined(SRC_DLL_NAME)
#error "Please define SRC_DLL_NAME"
#endif

#define RVA(T,B,O) ((T)(O+(char *)B))

#define T(X) L##X
#define ADD_DLL_EXT(X) T(X)".dll"
#define STR(X) #X
#define MAKE_DLL_NAME(X) ADD_DLL_EXT(STR(X))

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

static HANDLE WINAPI CreateFile2Impl(
        PCWSTR FileName,
        ULONG DesiredAccess,
        ULONG ShareMode,
        ULONG CreationDisposition,
        PCREATEFILE2_EXTENDED_PARAMETERS ExtendedParameters)
{
        if (ExtendedParameters) {
                if (ExtendedParameters->dwSize < sizeof(CREATEFILE2_EXTENDED_PARAMETERS)) {
                        SetLastError(STATUS_INVALID_PARAMETER);
                        return INVALID_HANDLE_VALUE;
                }

                ULONG FlagsAndAttributes = ExtendedParameters->dwFileFlags |
                                           ExtendedParameters->dwSecurityQosFlags |
                                           ExtendedParameters->dwFileAttributes;
                return CreateFile(
                        FileName,
                        DesiredAccess,
                        ShareMode,
                        ExtendedParameters->lpSecurityAttributes,
                        CreationDisposition,
                        FlagsAndAttributes,
                        ExtendedParameters->hTemplateFile);
        } else {
                return CreateFile(
                        FileName,
                        DesiredAccess,
                        ShareMode,
                        NULL,
                        CreationDisposition,
                        0,
                        NULL);
        }
}

static BOOL analyzeImportDescriptor( PIMAGE_IMPORT_DESCRIPTOR importDescriptor
                            , HMODULE baseAddress
                            , const char *apiName
                            , const char *newapiName
                            , ULONGLONG function )
{
    // gets the RVAs of OriginalFirstThunk & FirstThunk
    DWORD thunkILTRVA = importDescriptor->OriginalFirstThunk;
    DWORD thunkIATRVA = importDescriptor->FirstThunk;
    if (!thunkILTRVA || !thunkIATRVA)
        return FALSE;

    // getting lin. addr. of thunkILT
    PIMAGE_THUNK_DATA thunkILT = RVA(PIMAGE_THUNK_DATA, baseAddress, thunkILTRVA);
    PIMAGE_THUNK_DATA thunkIAT = RVA(PIMAGE_THUNK_DATA, baseAddress, thunkIATRVA);
    if (!thunkILT || !thunkIAT)
        return FALSE;

    for( ; thunkILT->u1.AddressOfData; thunkILT++, thunkIAT++ )
    {
       if (thunkILT->u1.Ordinal & IMAGE_ORDINAL_FLAG) // skip ordinals
          continue;

       // add base address to RVA of imported function's name to get the location of it
       PIMAGE_IMPORT_BY_NAME nameData = RVA(PIMAGE_IMPORT_BY_NAME, baseAddress, thunkILT->u1.AddressOfData);

       if (!stricmp(apiName, nameData->Name))
       {
           DWORD OldProtect = 0;
           if( newapiName )
           {
               // CreateFile2 -> CreateFileW
               int len = strlen(nameData->Name);
               if( len != strlen(newapiName) )
                    return FALSE; // ERROR: names must have the same length

               if( !VirtualProtect( nameData->Name, len, PAGE_READWRITE, &OldProtect ) )
                   return FALSE;
               strcpy( nameData->Name, newapiName );
               VirtualProtect( nameData->Name, len, OldProtect, &OldProtect );
           }
           else
           {
               if( !VirtualProtect( (void *)thunkIAT, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &OldProtect ) )
                   return FALSE;
               thunkIAT->u1.Function = function;
               VirtualProtect( (void *)thunkIAT, sizeof(IMAGE_THUNK_DATA), OldProtect, &OldProtect );
           }
           
           return TRUE;
       }
    }
    return FALSE;
}

static PIMAGE_IMPORT_DESCRIPTOR FindImportDescriptor( HMODULE baseAddress, const char * dllName )
{    
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)baseAddress;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
       return NULL;

    PIMAGE_NT_HEADERS peHeader = RVA(PIMAGE_NT_HEADERS, baseAddress, dosHeader->e_lfanew);
    if (peHeader->Signature != IMAGE_NT_SIGNATURE)
        return NULL;

    PIMAGE_OPTIONAL_HEADER64 pOptionalHeader = &peHeader->OptionalHeader;
    if( pOptionalHeader->Magic != 0x20B) // x64
        return NULL;

    PIMAGE_DATA_DIRECTORY pImportDirectory = &pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if( !pImportDirectory )
        return NULL;

    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = RVA(PIMAGE_IMPORT_DESCRIPTOR, baseAddress, pImportDirectory->VirtualAddress);

    for( int index = 0; importDescriptor[index].Characteristics != 0; index++ )
    {
       const char * nameOfDLL = RVA(const char *, baseAddress, importDescriptor[index].Name );
       if( !nameOfDLL )
           break;

       if( !stricmp( nameOfDLL, dllName ) )
           return &importDescriptor[index];
    }
    return NULL;
}

static HMODULE DLLBase = 0;
static PIMAGE_IMPORT_DESCRIPTOR K32Descr =  NULL;

static VOID CALLBACK OnLoadHook( ULONG NotificationReason,
                       PLDR_DLL_NOTIFICATION_DATA NotificationData,
                       PVOID Context )
{
    PCUNICODE_STRING BaseDllName = NotificationData->Loaded.BaseDllName;
    
    FRRINTF(stderr, "fwd_util.OnLoadHook: %Z NotificationReason %d\n", BaseDllName, NotificationReason );
    
    if( NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED )
        return;
   
    if( !_wcsnicmp( MAKE_DLL_NAME(SRC_DLL_NAME), BaseDllName->Buffer, BaseDllName->Length/sizeof(WCHAR)) ) // loading test.dll
    {
         // 1. step: change import CreateFile2 -> CreateFileW
         FRRINTF(stderr, "fwd_util.OnLoadHook.1\n" );
         
         // save DLLBase and K32Descr for step 2.
         DLLBase = (HMODULE)NotificationData->Loaded.DllBase;
         K32Descr = FindImportDescriptor( DLLBase, "KERNEL32.dll" );
         if( K32Descr )
         {
             FRRINTF(stderr, "fwd_util.OnLoadHook.2 FindImportDescriptor %x\n", K32Descr );
             BOOL r = analyzeImportDescriptor( K32Descr, DLLBase, "CreateFile2", "CreateFileW", 0 );
             FRRINTF(stderr, "fwd_util.OnLoadHook: CreateFile2 -> CreateFileW: %s\n", r ? "OK":"FAILED" );
         }
    }
}

static FARPROC WINAPI dllDelayLoadHook( unsigned dliNotify, PDelayLoadInfo pdli )
{
    if (dliNotify == dliNotePreLoadLibrary)
    {
        // 2. step: patch CreateFileW to CreateFile2Impl
        FRRINTF(stderr, "fwd_util.dllDelayLoadHook dliNotePreLoadLibrary %s\n", pdli->szDll);
        
        HMODULE hModule = LoadLibraryA( pdli->szDll );
        
        if( hModule == DLLBase && K32Descr )
        {
            BOOL r = analyzeImportDescriptor( K32Descr, DLLBase, "CreateFileW", NULL, (ULONGLONG)CreateFile2Impl );
            FRRINTF(stderr, "fwd_util.OnLoadHook: CreateFileW <- CreateFile2Impl: %s\n", r ? "OK":"FAILED" );
            
            // 3. step: copy data exports
            for( unsigned int i = 0; i < sizeof(descs)/sizeof(descs[0]); ++i )
            {
                 FARPROC pTable = GetProcAddress(DLLBase, descs[i].name);
                 if( pTable )
                 {
                     memmove( descs[i].table, (const void *)pTable, descs[i].size );
                 }
                 else
                 {
                     FRRINTF(stderr, "fwd_util.dllDelayLoadHook couldn't get symbol %s\n", descs[i].name); 
                 }
            }
            
            DLLBase = 0;
            K32Descr = NULL;
        }
        
        return (FARPROC)hModule;
    }
    return NULL;
}

PfnDliHook __pfnDliNotifyHook2 = dllDelayLoadHook;

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
    FRRINTF(stderr, "fwd_util.DllMain: %x %d\n", hinstDLL, fdwReason);
    
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