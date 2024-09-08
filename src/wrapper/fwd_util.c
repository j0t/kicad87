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

#define RVA(T,B,O) ((T)(O+(char *)B))

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
           if( newapiName )
           {
               // CreateFile2 -> CreateFileW
               if( strlen(apiName) != strlen(newapiName) )
                    return FALSE; // ERROR: names must have the same length
                
               strcpy( nameData->Name, newapiName );
           }
           else               
               thunkIAT->u1.Function = function;
           
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

    if( !_wcsnicmp( L"test.DLL", BaseDllName->Buffer, BaseDllName->Length/sizeof(WCHAR)) ) // loading test.dll
    {
         // 1. step: change import CreateFile2 -> CreateFileW
         
         // save DLLBase and K32Descr for step 2.
         DLLBase = (HMODULE)NotificationData->Loaded.DllBase;         
         K32Descr = FindImportDescriptor( DLLBase, "KERNEL32.dll" );
         if( K32Descr )
         {
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
        HMODULE hModule = LoadLibraryA(pdli->szDll);
        
        if( hModule == DLLBase && K32Descr )
        {
            BOOL r = analyzeImportDescriptor( K32Descr, DLLBase, "CreateFileW", NULL, (ULONGLONG)CreateFile2Impl );
            FRRINTF(stderr, "fwd_util.OnLoadHook: CreateFileW <- CreateFile2Impl: %s\n", r ? "OK":"FAILED" );
            
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