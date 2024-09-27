extern "C" __declspec(dllimport) void _CallFoo();

extern "C" __declspec(dllexport) void CallFoo()
{
    _CallFoo(); // if should be deffered load
}
