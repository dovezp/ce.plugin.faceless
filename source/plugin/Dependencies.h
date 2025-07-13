#pragma once

#include "ce/cepluginsdk.h"
#include "ntdll/ntdll.h"

#include <cstdio>
#include <vector>
#include <string>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <queue>

#define CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <windowsx.h>
#include <wincrypt.h>
#include <winnt.h>
#include <intrin.h>
#include <malloc.h>
#include <tchar.h>
#include <dwmapi.h>
#include <oleacc.h>
#include <commctrl.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#ifdef _WIN64
#pragma comment(lib, "ce/lua53-64.lib")
#pragma comment(lib,"ntdll/ntdll_x64.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "oleacc.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Comctl32.lib")
#else //x86
#pragma comment(lib, "ce/lua53-32.lib")
#pragma comment(lib,"ntdll/ntdll_x86.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "oleacc.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Comctl32.lib")
#endif

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define THIS ((HINSTANCE)&__ImageBase)
