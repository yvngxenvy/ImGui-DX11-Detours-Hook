# ImGui Hook for DX11 With Uninjecting support

This hook creates a fake window and fetches the SwapChain VFTable (Virtual Function Table). After it does this, it gets the "Present" method at index 8 and hooks it using Microsoft Detours. Present is a function that presents a rendered image to the user. So, if we use ImGui methods before this function calls, we can get a rendered ImGui Window. We then hook WndProc so we can handle ImGui inputs and call ImGui_ImplWin32_WndProcHandler. 

* All you have to do to use this is call `UIHook.Initialize();` after linking the Main Files. All you need is Microsoft Detours, ImGui DX11 library, and UserInterfaceHook cpp/hpp files.
* After this, you call UIHook.AddWindowInfo(...) to add a window. you can have multiple windows.

**This is all under an MIT liscense, so feel free to do anything with it! However, it would be appreciated if you credited me as it would support me heavily.**

> *For anyone wondering, this is the same hook I use in Logical (Rocket League Mod)*

> *A lot of the code in this project was taken from [ImGui-DirectX-11-Kiero-Hook](https://github.com/rdbo/ImGui-DirectX-11-Kiero-Hook/blob/master/ImGui%20DirectX%2011%20Kiero%20Hook/main.cpp).*
