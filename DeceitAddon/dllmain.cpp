#include <thread>
#include <Windows.h>
#include "DirectX.h"
#include "Main.h"

HMODULE g_module;
uintptr_t initThreadHandle;

bool hooked = true;

void OnExit() noexcept {
	DirectXHook::unHook();
	Main::Deinit();
	if (hooked) {
		hooked = false;
	}
	CloseHandle((HANDLE)initThreadHandle);
}

DWORD WINAPI MainThread(LPVOID param) {
	Main::Init();
	DirectXHook::HookDX11();

	while (!GetAsyncKeyState(VK_END)) {
		Sleep(1);
	}
	return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	g_module = hModule;
	DisableThreadLibraryCalls(hModule);
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		std::atexit(OnExit);
		initThreadHandle = _beginthreadex(nullptr, 0, (_beginthreadex_proc_type)MainThread, hModule, 0, nullptr);
		FreeLibrary(hModule);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		OnExit();
		break;
	}
	return TRUE;
}
