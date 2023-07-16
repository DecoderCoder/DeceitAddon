#include "DirectX.h"
#include "Main.h"
#include "Utils.h"

uint64_t oPresentDX11 = NULL;
uint64_t oResizeBufferDX11 = NULL;
ID3D11Device* pDevice = NULL;
IDXGISwapChain* gpSwapChain = NULL;
ID3D11DeviceContext* pContext = NULL;


HWND window = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;
WNDPROC oWndProc = NULL;

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
	DWORD wndProcId;
	GetWindowThreadProcessId(handle, &wndProcId);
	if (GetCurrentProcessId() != wndProcId) {
		return true;
	}
	window = handle;
	return false;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool DirectXHook::HookDX11() {
	//UltHook.RestoreSysDll("kernel32.dll");
	//UltHook.RestoreSysDll("KernelBase.dll");
	//UltHook.RestoreSysDll("D3DCompiler_43.dll");
	//UltHook.RestoreSysDll("d3dcompiler_47.dll");	
	//UltHook.RestoreSysDll("dxgi.dll");
	//UltHook.RestoreSysDll("d3d11.dll");
	//UltHook.RestoreSysDll("DXCore.dll");

	if (Inited)
		return true;
	WNDCLASSEX windowClass;
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.hIcon = NULL;
	windowClass.hCursor = NULL;
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = TEXT("Decode");
	windowClass.hIconSm = NULL;
	::RegisterClassEx(&windowClass);
	window = ::CreateWindow(windowClass.lpszClassName, TEXT("Decode"), WS_OVERLAPPEDWINDOW, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, windowClass.hInstance, NULL);

	HMODULE libD3D11;
	if ((libD3D11 = ::GetModuleHandle(TEXT("d3d11.dll"))) == NULL)
	{
		MessageBoxA(0, "(libD3D11 = ::GetModuleHandle(TEXT(\"d3d11.dll\"))) == NULL", "", 0);
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return false;
	}

	void* D3D11CreateDeviceAndSwapChain;
	if ((D3D11CreateDeviceAndSwapChain = ::GetProcAddress(libD3D11, "D3D11CreateDeviceAndSwapChain")) == NULL)
	{
		MessageBoxA(0, "D3D11CreateDeviceAndSwapChain = ::GetProcAddress(libD3D11, \"D3D11CreateDeviceAndSwapChain\")) == NULL", "", 0);
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return false;
	}

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

	DXGI_RATIONAL refreshRate;
	refreshRate.Numerator = 170;
	refreshRate.Denominator = 1;

	DXGI_MODE_DESC bufferDesc;
	bufferDesc.Width = GetSystemMetrics(SM_CXSCREEN);
	bufferDesc.Height = GetSystemMetrics(SM_CYSCREEN);
	bufferDesc.RefreshRate = refreshRate;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = window;
	swapChainDesc.Windowed = true; // false
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	IDXGISwapChain* swapChain;
	ID3D11Device* device;
	ID3D11DeviceContext* context;

	auto CreateDeviceResponse = ((long(__stdcall*)(
		IDXGIAdapter*,
		D3D_DRIVER_TYPE,
		HMODULE,
		UINT,
		const D3D_FEATURE_LEVEL*,
		UINT,
		UINT,
		const DXGI_SWAP_CHAIN_DESC*,
		IDXGISwapChain**,
		ID3D11Device**,
		D3D_FEATURE_LEVEL*,
		ID3D11DeviceContext**))(D3D11CreateDeviceAndSwapChain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, &featureLevel, &context);

	if (CreateDeviceResponse != S_OK)
	{
		MessageBoxA(0, std::to_string(CreateDeviceResponse).c_str(), "D3D11CreateDeviceAndSwapChain", 0);
		Beep(900, 100);
		::DestroyWindow(window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		return false;
	}


	uint64_t* g_methodsTable = (uint64_t*)::calloc(205, sizeof(uint64_t));
	::memcpy(g_methodsTable, *(uint64_t**)swapChain, 18 * sizeof(uint64_t));
	::memcpy(g_methodsTable + 18, *(uint64_t**)device, 43 * sizeof(uint64_t));
	::memcpy(g_methodsTable + 18 + 43, *(uint64_t**)context, 144 * sizeof(uint64_t));
	uintptr_t target = g_methodsTable[8]; // [8]   Present

	//if (*(BYTE*)target != 0xE9) // for hook with saving overlay hooks
	//oPresentDX11 = UltHook.AddEzHook((DWORD64)target, 5, (DWORD64)&DirectXHook::Hooked_PresentDX11);
	//else
	//	oPresentDX11 = UltHook.AddEzHook((DWORD64)((int)target + 5), 6, (DWORD64)&DirectXHook::Hooked_PresentDX11);
	//MessageBoxA(0, to_hex((int)oPresentDX11).c_str(), "opresentDX11", 0);

	PIMAGE_NT_HEADERS NTHeaders = RtlImageNtHeader(GetModuleHandle(NULL));
	PIMAGE_NT_HEADERS DXHeaders = RtlImageNtHeader(GetModuleHandle(L"dxgi.dll"));

	auto hook = &DirectXHook::Hooked_PresentDX11;

	//if (*(BYTE*)(target) == 0xE9) {
	//	// Restore orig
	//	DWORD old;
	//	char orig_dx[] = { 0x48, 0x89, 0x5C, 0x24, 0x20, 0x55, 0x56, 0x57, 0x48, 0x8D, 0x6C, 0x24, 0xB9 };
	//	VirtualProtect((void*)target, sizeof(orig_dx), 0x40, &old);
	//	memcpy((void*)target, &orig_dx, sizeof(orig_dx));
	//	VirtualProtect((void*)target, sizeof(orig_dx), old, &old);
	//}

	oPresentDX11 = (uintptr_t)HookVTableFunction(swapChain, &DirectXHook::Hooked_PresentDX11, 8);
	//oPresentDX11 = UltHook.AddEzHook((uintptr_t)target, 28, (uintptr_t)hook);

	//std::this_thread::sleep_for(500ms);

	/*while (!Inited) {
		Beep(50, 10);
		Utils::Log(" > Renderer: Rehook");
		detour->reHook();
	}*/
	return true;
}

HRESULT __stdcall DirectXHook::Hooked_PresentDX11(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {

	//if (GetAsyncKeyState(VK_HOME))
	//	Inited = false;	
	if (!Inited)
	{
		auto res = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);
		if (res == S_OK && pDevice)
		{
			//Images_Manager = Renderer_Image_Manager(pDevice);
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			RECT rect;
			GetWindowRect(window, &rect);
			Render::RenderWidth = rect.right - rect.left;
			Render::RenderHeight = rect.bottom - rect.top;

			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();

			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			Inited = true;
		}
		else
			return ((PresentDX11)oPresentDX11)(pSwapChain, SyncInterval, Flags);

		gpSwapChain = pSwapChain;
		oResizeBufferDX11 = (uintptr_t)HookVTableFunction(gpSwapChain, &DirectXHook::Hooked_ResizeBufferDX11, 13);
		/*PLH::VFuncMap redirect = { {13, (uint64_t)&DirectXHook::Hooked_ResizeBufferDX11} };

		h.reset(new PLH::VTableSwapHook((uint64_t)pSwapChain, redirect));
		if (!h->hook()) {
			MessageBoxA(0, "Failed to hook", "", 0);
		}*/

		//HookVTableFunction(pSwapChain, &DirectXHook::Hooked_ResizeBufferDX11, 13);

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX11_Init(pDevice, pContext);

		//FreeLibrary(GetModuleHandle(L"D3DCompiler_47.dll"));
	}
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	Main::MainWindow();

	ImGui::Render();
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return ((PresentDX11)oPresentDX11)(pSwapChain, SyncInterval, Flags);
}

HWND GetHwndProc()
{
	HWND g_hWindow = GetTopWindow(NULL);
	DWORD64 currentPID = GetCurrentProcessId();
	do
	{
		char title[256];
		if ((GetWindowTextA(g_hWindow, title, 256) > 0) && (IsWindowVisible(g_hWindow)))
		{
			DWORD procId;
			GetWindowThreadProcessId(g_hWindow, &procId);

			if (procId == currentPID)
			{
				return g_hWindow;
			}
		}

		g_hWindow = GetNextWindow(g_hWindow, GW_HWNDNEXT);
	} while (g_hWindow);
	return NULL;
}

bool DirectXHook::unHook() {
	SetWindowLongPtr(GetHwndProc(), GWLP_WNDPROC, (LONG_PTR)oWndProc);
	if (oPresentDX11)
		(uintptr_t)HookVTableFunction(gpSwapChain, (void*)oPresentDX11, 8);
	//UltHook.RemoveEzHook(oPresentDX11);

	if (oResizeBufferDX11)
		HookVTableFunction(gpSwapChain, (void*)oResizeBufferDX11, 13);
	return false;
}

HRESULT  __stdcall DirectXHook::Hooked_ResizeBufferDX11(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	if (mainRenderTargetView) {
		//ImGui::DestroyContext();
		//SetWindowLongPtr(GetHwndProc(), GWLP_WNDPROC, (LONG_PTR)oWndProc);
		pContext->OMSetRenderTargets(0, 0, 0);
		mainRenderTargetView->Release();
	}
	HRESULT hr = ((ResizeBuffer)oResizeBufferDX11)(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
	ID3D11Texture2D* pBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
	// Perform error handling here!
	pDevice->CreateRenderTargetView(pBuffer, NULL, &mainRenderTargetView);

	/*pDevice->GetImmediateContext(&pContext);
	DXGI_SWAP_CHAIN_DESC sd;
	pSwapChain->GetDesc(&sd);
	window = sd.OutputWindow;
	RECT rect;
	GetWindowRect(window, &rect);
	Render::RenderWidth = rect.right - rect.left;
	Render::RenderHeight = rect.bottom - rect.top;
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
	(WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);*/

	// Perform error handling here!
	pBuffer->Release();
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	D3D11_VIEWPORT vp;
	vp.Width = Width;
	vp.Height = Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pContext->RSSetViewports(1, &vp);
	return hr;
}