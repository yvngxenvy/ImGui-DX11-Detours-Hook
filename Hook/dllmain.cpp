#include "UserInterfaceHook.hpp"

bool bWindowOpen = true;

void ImGuiContent() {
    if (ImGui::BeginChild("DebugChild", {})) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("FPS: %.2f", io.Framerate);

        static int testInt;
        ImGui::InputInt("Integer", &testInt);
        ImGui::Text("Your integer is %d", testInt);

        ImGui::EndChild();
    }
}

void Initialize(HMODULE hModule) {
    DisableThreadLibraryCalls(hModule);
    AllocConsole();

    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    
    LOG("Initialized Console");

    UIHook.AddWindowInfo("ImGui Template Window", &bWindowOpen, { 500.f, 500.f }, ImGuiContent, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
    UIHook.AddWindowInfo("ImGui Template Window 2", &bWindowOpen, { 200.f, 200.f }, ImGuiContent, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    UIHook.Initialize();
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Initialize), nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        FreeConsole();
        break;
    }
    return TRUE;
}