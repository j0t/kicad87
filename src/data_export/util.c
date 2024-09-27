#include <windows.h>
#include <stdio.h>

static void FooA() { fprintf(stderr, "FoooA!\n"); }
static void BarA() { fprintf(stderr, "BarA!\n");  }

static void FooB() { fprintf(stderr, "FoooB!\n"); }
static void BarB() { fprintf(stderr, "BarB!\n");  }

extern "C" __declspec(dllexport) void CallFoo() { fprintf(stderr, "CallFoo!\n"); }

typedef void (*fptr)();

fptr Table_133[] = { FooA, BarA };
fptr Table_134[] = { FooB, BarB };

// "??_7FOOTPRINT_INFO@@6B@"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    fprintf(stderr,"util.DllMain: %x %d\n", hinstDLL, fdwReason);
    return TRUE;
}