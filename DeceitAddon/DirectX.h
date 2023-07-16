#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "d3d9.h"
#include "d3d11.h"
#include <dxgi.h>
#include <stdint.h>
#include "d3dx11tex.h"
#include "Hooks/ultimate_hooks.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Advapi32.lib")

extern "C" NTSYSAPI PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(PVOID Base);

typedef HRESULT(__stdcall* PresentDX11) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffer)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);


typedef HRESULT(WINAPI* PresentDX9) (LPDIRECT3DDEVICE9 Device, CONST RECT* pSrcRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion);
typedef HRESULT(__stdcall* PresentDX11) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffer)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

namespace DirectXHook {
	inline bool Inited = false;

	bool HookDX11();
	bool unHook();

	HRESULT __stdcall Hooked_PresentDX11(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
	HRESULT  __stdcall Hooked_ResizeBufferDX11(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
}
