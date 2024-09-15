#pragma once
#define WIN32_LEAN_AND_MEAN

// Define this if you want no logs in your console when events are finished (HookWndProc, SetUpImGui, etc)
// #define NO_LOGS 

#include <Windows.h>
#include <iostream>
#include <functional>

#include <detours.h>

#ifndef NO_LOGS
#define LOG(x) std::cout << "[*] " << x << std::endl;
#else
#define LOG(x) // No-op (empty definition)
#endif