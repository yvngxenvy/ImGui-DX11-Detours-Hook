#pragma once
#include "../pch.hpp"
#include <dxgi.h>
#include <d3d11.h>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_internal.h"

enum DirectXHookStatus {
	ModuleNotFoundError,
	ProcAddrNotFoundError,
	UnknownError,

	Success,
};

typedef HRESULT(*PresentType) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags); // Function for presenting a rendered image for DirectX

class UserInterfaceHook
{
private:
	static inline bool bDetoured;
	static inline bool bShouldInitialize;
	static inline bool bInitialized;
	static inline bool bReadyToPresent;
	static inline bool bContextCreated; // Bool if ImGui Context is created. This is a nescessary check for WndProc

	static inline HWND FakeWindow;
	static inline WNDCLASSEX WindowClass;

	static inline uint64_t* VFTable;
	static inline PresentType Present;

	static inline HWND GameWindow; // The window that the DLL is injected into
	static inline WNDPROC oWndProc;
	static inline ID3D11Device* Device;
	static inline ID3D11DeviceContext* DeviceContext;
	static inline ID3D11RenderTargetView* RenderTargetView;

	// New Variables For Dynamic GUI Loading
	static inline std::string WindowName;
	static inline bool* bOpenVariable;
	static inline std::pair<float, float> Size;
	static inline std::function<void()> ContentFn;

	HANDLE InternalThread;
private:
	static void CreateFakeClass();
	static void DestroyFakeClass();

	static DirectXHookStatus FetchMethodsTable();
	static DirectXHookStatus InitializeInternal();

	static HRESULT PresentDetour(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
	static void AttachDetour(PresentType DetourFunction, PVOID Detour);
	static LRESULT WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static bool HookWndProc(IDXGISwapChain* SwapChain, LONG_PTR NewWndProc);
	static void SetUpImGui();

	static void ThreadCheck();

public:
	static std::string GetStringStatus(DirectXHookStatus Error);
	static HWND GetGameWindow();

	static void SetGuiInfo(std::string _WindowName, bool* _bOpenVariable, std::pair<float, float> _Size, std::function<void()> _ContentFn);
	static bool IsInfoSet();

	static void Initialize();

	UserInterfaceHook();
	~UserInterfaceHook();
};

extern class UserInterfaceHook UIHook;