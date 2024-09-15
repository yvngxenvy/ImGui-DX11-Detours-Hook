#include "UserInterfaceHook.hpp"

namespace ImGuiSettings {
	const bool bSavedFiles = false; // .Ini file for imgui, this saves Window Position and Size.
	const bool bCanResize = false; // Adds a resize bar thing at the bottom right of a window
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool UserInterfaceWindowInfo::IsOpen()
{
	return *bOpenVariable;
}

void UserInterfaceWindowInfo::Render()
{
	if (IsInfoSet()) {
		if (IsOpen()) {
			// Dynamically Render
			ImGui::SetNextWindowSize(ImVec2(Size.first, Size.second), (ImGuiSettings::bCanResize ? ImGuiCond_Once : ImGuiCond_Always));

			ImGui::Begin(WindowName.c_str(), bOpenVariable, Flags 
				| (ImGuiSettings::bSavedFiles ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoSavedSettings) 
				| (ImGuiSettings::bCanResize ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoResize));

			// Function passed by user
			ContentFn();

			ImGui::End();
		}
	}
}

void UserInterfaceWindowInfo::SetOpen(bool bNewValue)
{
	*bOpenVariable = bNewValue;
}

bool UserInterfaceWindowInfo::IsInfoSet()
{
	if (bOpenVariable && ContentFn) {
		return true;
	}

	return false;
}

UserInterfaceWindowInfo::UserInterfaceWindowInfo() : WindowName("Blank"), bOpenVariable(nullptr), Size({ 0.f, 0.f }), ContentFn(nullptr), Flags(ImGuiWindowFlags_None) {}

UserInterfaceWindowInfo::UserInterfaceWindowInfo(std::string _WindowName, bool* _bOpenVariable, std::pair<float, float> _Size, std::function<void()> _ContentFn, ImGuiWindowFlags _Flags) : WindowName(_WindowName), bOpenVariable(_bOpenVariable), Size(_Size), ContentFn(_ContentFn), Flags(_Flags) {}

UserInterfaceWindowInfo::~UserInterfaceWindowInfo() {}

void UserInterfaceHook::CreateFakeClass()
{
	// Creates a fake window/class so we can retrieve DirectX info from it
	::RegisterClassEx(&WindowClass);

	FakeWindow = ::CreateWindow(WindowClass.lpszClassName, L"Envy DirectX Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, WindowClass.hInstance, NULL);
}

DirectXHookStatus UserInterfaceHook::FetchMethodsTable()
{
	// Will fetch the VFTable for IDXGISwapChain. In this VFTable is a method "Present" which we need to hook and detour

	HMODULE libD3D11;
	if ((libD3D11 = ::GetModuleHandle(L"d3d11.dll")) == NULL)
	{
		DestroyFakeClass();
		return DirectXHookStatus::ModuleNotFoundError;
	}

	void* D3D11CreateDeviceAndSwapChain;
	if ((D3D11CreateDeviceAndSwapChain = ::GetProcAddress(libD3D11, "D3D11CreateDeviceAndSwapChain")) == NULL)
	{
		DestroyFakeClass();
		return DirectXHookStatus::ProcAddrNotFoundError;
	}

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

	DXGI_RATIONAL refreshRate;
	refreshRate.Numerator = 60;
	refreshRate.Denominator = 1;

	DXGI_MODE_DESC bufferDesc;
	bufferDesc.Width = 100;
	bufferDesc.Height = 100;
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
	swapChainDesc.OutputWindow = FakeWindow;
	swapChainDesc.Windowed = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	IDXGISwapChain* swapChain;
	ID3D11Device* device;
	ID3D11DeviceContext* context;

	if (((long(__stdcall*)(
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
		ID3D11DeviceContext**))(D3D11CreateDeviceAndSwapChain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, &featureLevel, &context) < 0)
	{
		DestroyFakeClass();
		return DirectXHookStatus::UnknownError;
	}

	VFTable = (uint64_t*)::calloc(205, sizeof(uint64_t));
	::memcpy(VFTable, *(uint64_t**)swapChain, 18 * sizeof(uint64_t));
	::memcpy(VFTable + 18, *(uint64_t**)device, 43 * sizeof(uint64_t));
	::memcpy(VFTable + 18 + 43, *(uint64_t**)context, 144 * sizeof(uint64_t));

	swapChain->Release();
	swapChain = NULL;

	device->Release();
	device = NULL;

	context->Release();
	context = NULL;

	DestroyFakeClass();

	return DirectXHookStatus::Success;
}

void UserInterfaceHook::DestroyFakeClass()
{
	// Destroys fake class because we do not need anymore

	::DestroyWindow(FakeWindow);
	::UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
}

std::string UserInterfaceHook::GetStringStatus(DirectXHookStatus Error)
{
	// For debugging or general purposes, we can get the string of DirectXHookStatus
	
	switch (Error) {
	case DirectXHookStatus::ModuleNotFoundError:
		return "ModuleNotFound";
	case DirectXHookStatus::ProcAddrNotFoundError:
		return "ProcAddrNotFound";
	case DirectXHookStatus::UnknownError:
		return "UnknownError";
	case DirectXHookStatus::Success:
		return "Success";
	}
}

DirectXHookStatus UserInterfaceHook::InitializeInternal()
{
	// Internal Initialize to get VFTable

	CreateFakeClass();
	return FetchMethodsTable();
}

HRESULT UserInterfaceHook::PresentDetour(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	// This is the most important part of the whole hook, this is the trampoline function for Present. Everything in here will run BEFORE present, as long as we call the original function (Present). 

	if (bReadyToPresent == false) {
		// Basic initialization, we need to hook WndProc so we can call ImGui_ImplWin32_WndProcHandler and handle inputs for imgui
		
		if (HookWndProc(pSwapChain, (LONG_PTR)WndProc) == false) {
			return Present(pSwapChain, SyncInterval, Flags);
		};
		SetUpImGui();

		bReadyToPresent = true;
	}

	// Start Render
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	for (auto Info : WindowInfos) {
		Info.Render();
	}

	// End Render
	ImGui::EndFrame();
	ImGui::Render();
	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return Present(pSwapChain, SyncInterval, Flags);
}

void UserInterfaceHook::AttachDetour(PresentType DetourFunction, PVOID Detour)
{
	// Function that attaches a detour

	Present = DetourFunction;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&reinterpret_cast<PVOID&>(Present), Detour);
	DetourTransactionCommit();
	bDetoured = true;
}

LRESULT UserInterfaceHook::WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// WndProc trampoline

	// I don't know if theres any method in the ImGui library that can check if any window is currently opened, but if there is, please lmk!
	if (IsAnyWindowOpen() == false) {
		return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
	}

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	if (bContextCreated) {
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse)
		{
			return TRUE;
		}
	}

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool UserInterfaceHook::HookWndProc(IDXGISwapChain* SwapChain, LONG_PTR NewWndProc)
{
	// Function that gets the ID3D11Device which is used to get the RenderTargetView
	// The GameWindow is grabbed from the SwapChain which we call GetBuffer and that returns DXGI_SWAP_CHAIN_DESC with "OutputWindow"

	HRESULT hr = SwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&Device);
	if (SUCCEEDED(hr)) {
		DXGI_SWAP_CHAIN_DESC sd;
		ID3D11Texture2D* pBackBuffer;

		Device->GetImmediateContext(&DeviceContext);
		SwapChain->GetDesc(&sd);

		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		Device->CreateRenderTargetView(pBackBuffer, NULL, &RenderTargetView);

		GameWindow = sd.OutputWindow;
		oWndProc = (WNDPROC)SetWindowLongPtr(GameWindow, GWLP_WNDPROC, NewWndProc);

		pBackBuffer->Release();
		LOG("Hooked WndProc");

		return true;
	}

	return false;
}

void UserInterfaceHook::SetUpImGui()
{
	// Set up ImGui context

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	if (ImGuiSettings::bSavedFiles == false) {
		io.IniFilename = NULL;
	}

	io.WantCaptureKeyboard = false;
	io.WantCaptureMouse = false;
	ImGui_ImplWin32_Init(GameWindow);
	ImGui_ImplDX11_Init(Device, DeviceContext);

	bContextCreated = true;

	{
		// This is where you should load images, fonts, etc
		// io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Segoeuib.ttf", 17.f)
	}

	LOG("Set Up ImGui");
}

void UserInterfaceHook::ThreadCheck()
{
	// Periodic check in a different thread other than the game thread and the main thread.
	// There is probably a better way to do this, if you know of one, please lmk.

	do
	{
		if (bShouldInitialize) {
			DirectXHookStatus ResultStatus = InitializeInternal();
			if (ResultStatus == DirectXHookStatus::Success)
			{
				AttachDetour(reinterpret_cast<PresentType>(VFTable[8]), reinterpret_cast<PVOID>(PresentDetour));

				LOG("Internal Initialize " << GetStringStatus(ResultStatus));

				bInitialized = true;
			}
			else {
				LOG("Failed internal initialize because " << GetStringStatus(ResultStatus));
			}
		}
	} while (bInitialized == false);
}

void UserInterfaceHook::AddWindowInfo(std::string WindowName, bool* bOpenVariable, std::pair<float, float> Size, std::function<void()> ContentFn, ImGuiWindowFlags Flags)
{
	WindowInfos.push_back(UserInterfaceWindowInfo(WindowName, bOpenVariable, Size, ContentFn, Flags));
}

bool UserInterfaceHook::IsWindowInfoSet()
{
	if (WindowInfos.empty()) {
		return false;
	};

	for (auto Info : WindowInfos) {
		if (Info.IsInfoSet() == false) {
			return false;
		}
	}
	
	return true;
}

void UserInterfaceHook::Initialize()
{
	// Public function to let the internal know when you should start the hook process

	if (IsWindowInfoSet()) {
		bShouldInitialize = true;
	}
	else {
		LOG("Unable to start initialization: no ImGui info Set!");
	}
}

HWND UserInterfaceHook::GetGameWindow()
{
	// Public function for if you need the game's HWND at any point. This is good because it is directly grabbed from DirectX.

	return GameWindow;
}

bool UserInterfaceHook::IsAnyWindowOpen()
{
	for (auto Info : WindowInfos) {
		if (Info.IsOpen()) {
			return true;
		}
	}

	return false;
}

UserInterfaceHook::UserInterfaceHook() : InternalThread(NULL)
{
	// Constructor

	InternalThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ThreadCheck), nullptr, 0, nullptr);
	VFTable = NULL;

	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = DefWindowProc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = GetModuleHandle(NULL);
	WindowClass.hIcon = NULL;
	WindowClass.hCursor = NULL;
	WindowClass.hbrBackground = NULL;
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpszClassName = L"EnvyWindow";
	WindowClass.hIconSm = NULL;
}

UserInterfaceHook::~UserInterfaceHook()
{
	// Deconstructor

	if (VFTable) {
		::free(VFTable);
		VFTable = NULL;
	}

	// Detaches detour
	if (bDetoured) {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&reinterpret_cast<PVOID&>(Present), reinterpret_cast<PVOID>(PresentDetour));
		DetourTransactionCommit();
	}

	// Destroy ImGui stuff
	if (bReadyToPresent) { // bReadyToPresent is a variable after the initialization of imgui, so this is a good signal/sign that imgui is setup
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	if (oWndProc) { // Old WndProc
		SetWindowLongPtrW(GameWindow, GWLP_WNDPROC, (LONG_PTR)oWndProc);
	}

	if (InternalThread) {
		CloseHandle(InternalThread);
	}
}

class UserInterfaceHook UIHook;