#include <windows.h>
#include <stdio.h>

static void Foo()
{
    fprintf(stderr, "Fooo!\n");
}

static void Bar()
{
    fprintf(stderr, "Bar!\n");
}

typedef void (*fptr)();

__declspec(dllexport) fptr Table[] = { Foo, Bar };

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    fprintf(stderr,"util.DllMain: %x %d\n", hinstDLL, fdwReason);
    return TRUE;
}