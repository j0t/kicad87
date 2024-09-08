#include <stdio.h>
#include <windows.h>

extern "C" __declspec(dllexport) HANDLE Open()
{
    WCHAR buf[MAX_PATH];
    
    CREATEFILE2_EXTENDED_PARAMETERS extParams = {0};
    extParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    
    return CreateFile2(buf, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extParams);
}

DWORD WINAPI ModifiedGetCurrentProcessId()
{
    return 777;
}

#define RVA(T,B,O) ((T)(O+(char *)B))

BOOL analyzeImportDescriptor( PIMAGE_IMPORT_DESCRIPTOR importDescriptor, HMODULE baseAddress, const char *apiName )
{
    DWORD(WINAPI *procPtr)() = ModifiedGetCurrentProcessId;
    
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
       
       if (!strcmp(apiName, nameData->Name))
       {
           thunkIAT->u1.Function = (ULONGLONG)procPtr; // patch
           return TRUE;
       }
    }
    return FALSE;
}

BOOL parsingImports( HMODULE baseAddress, const char *nameOfAPI )
{
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)baseAddress;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
       return FALSE;
     
    PIMAGE_NT_HEADERS peHeader = RVA(PIMAGE_NT_HEADERS, baseAddress, dosHeader->e_lfanew);
    if (peHeader->Signature != IMAGE_NT_SIGNATURE)
       return FALSE;
   
    PIMAGE_OPTIONAL_HEADER64 pOptionalHeader = &peHeader->OptionalHeader;
    if( pOptionalHeader->Magic != 0x20B) // x64
        return FALSE;
   
    PIMAGE_DATA_DIRECTORY pImportDirectory = &pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if( !pImportDirectory )
        return FALSE;
     
    DWORD descriptorStartRVA = pImportDirectory->VirtualAddress;
     
    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = RVA(PIMAGE_IMPORT_DESCRIPTOR, baseAddress, descriptorStartRVA);
     
    for( int index = 0; importDescriptor[index].Characteristics != 0; index++ )
    {
       const char * nameOfDLL = RVA(const char *, baseAddress, importDescriptor[index].Name );
       if( !nameOfDLL )
           break;
      
       if( !strcmp( nameOfDLL, "KERNEL32.dll") )
           return analyzeImportDescriptor( &importDescriptor[index], baseAddress, nameOfAPI );
    }
    return FALSE;
}

HINSTANCE Hinstance = 0;
BOOL hooking( const char *nameOfAPI )
{
    HMODULE addrOfModule = GetModuleHandle("test.dll");
    return parsingImports(addrOfModule, nameOfAPI);
}

extern "C" __declspec(dllexport) int Hello()
{
    BOOL hook = hooking("GetCurrentProcessId");
    return GetCurrentProcessId();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch (fdwReason)
  {
    case DLL_PROCESS_ATTACH:
      Hinstance = hinstDLL;
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      break;
  }

  return TRUE;
}