#include "AntiScreenCapture.h"
#include "Logger.h"
#include <psapi.h>
#include <algorithm>
#include <sstream>

#define CLASS_NAME_TAG __FILE__

#ifndef EVENT_OBJECT_CREATE
#define EVENT_OBJECT_CREATE 0x8000
#endif

#ifndef EVENT_OBJECT_DESTROY
#define EVENT_OBJECT_DESTROY 0x8001
#endif

#ifndef EVENT_OBJECT_SHOW
#define EVENT_OBJECT_SHOW 0x8002
#endif

#ifndef OBJID_WINDOW
#define OBJID_WINDOW 0x00000000
#endif

#ifndef CHILDID_SELF
#define CHILDID_SELF 0
#endif

AntiScreenCapture* AntiScreenCapture::HookInstance = nullptr;

AntiScreenCapture::AntiScreenCapture()
	: SupportedMethods(0)
	, bIsInitialized(false)
	, WindowEventHook(nullptr)
	, bIsHookActive(false)
	, HookTargetProcessId(0)
	, MessageWindow(nullptr)
	, HookMessageThread(nullptr)
	, bHookMessageThreadActive(false)
{
}

AntiScreenCapture::~AntiScreenCapture()
{
	try
	{
		if (HookInstance == this)
		{
			HookInstance = nullptr;
		}

		Shutdown();
	}
	catch (const std::exception& Exception)
	{
		Log("[%s] Exception in destructor: %s", CLASS_NAME_TAG, Exception.what());
	}
}

bool AntiScreenCapture::Initialize()
{
	if (bIsInitialized)
	{
		return true;
	}

	bIsInitialized = true;
	Log("[%s] Initialized", CLASS_NAME_TAG);
	return true;
}

void AntiScreenCapture::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	StopWindowEventHook();

	std::lock_guard<std::mutex> Lock(ProtectedWindowsMutex);
	std::vector<HWND> handles_to_remove;
	for (const auto& pair : ProtectedWindows)
	{
		handles_to_remove.push_back(pair.first);
	}

	for (HWND handle : handles_to_remove)
	{
		RemoveProtectionInternal(handle);
	}
	ProtectedWindows.clear();

	bIsInitialized = false;
	Log("[%s] Shutdown", CLASS_NAME_TAG);
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::ApplyProtection(HWND WindowHandle, uint32_t Methods)
{
	if (!bIsInitialized)
	{
		return EProtectionResult::Failed;
	}

	if (!ValidateWindowHandle(WindowHandle))
	{
		return EProtectionResult::InvalidWindow;
	}

	std::lock_guard<std::mutex> Lock(ProtectedWindowsMutex);
	return ApplyProtectionInternal(WindowHandle, Methods);
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::ApplyProtectionInternal(HWND WindowHandle, uint32_t Methods)
{
	WindowInfo WinInfo = GetWindowInfo(WindowHandle);

	auto It = ProtectedWindows.find(WindowHandle);
	if (It != ProtectedWindows.end() && It->second.bIsActive)
	{
		return EProtectionResult::AlreadyProtected;
	}

	EProtectionResult FinalResult = EProtectionResult::Success;
	uint32_t AppliedMethods = 0;

	if (Methods & static_cast<uint32_t>(EProtectionMethod::DisplayAffinity))
	{
		EProtectionResult Result = ApplySetWindowDisplayAffinity(WindowHandle);
		if (Result == EProtectionResult::Success)
		{
			AppliedMethods |= static_cast<uint32_t>(EProtectionMethod::DisplayAffinity);
		}
		else
		{
			FinalResult = EProtectionResult::Failed;
		}
	}

	ProtectionStatus& Status = ProtectedWindows[WindowHandle];
	Status.WindowHandle = WindowHandle;
	Status.AppliedMethods = AppliedMethods;
	Status.LastResult = FinalResult;
	Status.LastUpdate = std::chrono::steady_clock::now();
	Status.RetryCount = 0;
	Status.bIsActive = (AppliedMethods != 0);
	Status.ProcessName = WinInfo.WindowTitle;
	Status.ProcessId = WinInfo.ProcessId;

	Log("[%s] ProtectionStatus: HWND=0x%p, AppliedMethods=%d, LastResult=%d, LastUpdate=%lld ms, RetryCount=%d, bIsActive=%s, ProcessName=%s, ProcessId=%d",
		CLASS_NAME_TAG,
		Status.WindowHandle,
		Status.AppliedMethods,
		Status.LastResult,
		Status.LastUpdate.time_since_epoch().count(),
		Status.RetryCount,
		Status.bIsActive ? "true" : "false",
		Status.ProcessName.c_str(),
		Status.ProcessId
	);
	return FinalResult;
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::RemoveProtection(HWND WindowHandle)
{
	if (!bIsInitialized)
	{
		return EProtectionResult::Failed;
	}

	if (!ValidateWindowHandle(WindowHandle))
	{
		return EProtectionResult::InvalidWindow;
	}

	std::lock_guard<std::mutex> Lock(ProtectedWindowsMutex);
	return RemoveProtectionInternal(WindowHandle);
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::RemoveProtectionInternal(HWND WindowHandle)
{
	auto It = ProtectedWindows.find(WindowHandle);
	if (It == ProtectedWindows.end())
	{
		return EProtectionResult::Success;
	}

	ProtectionStatus& Status = It->second;
	EProtectionResult FinalResult = EProtectionResult::Success;

	if (Status.AppliedMethods & static_cast<uint32_t>(EProtectionMethod::DisplayAffinity))
	{
		EProtectionResult Result = RemoveSetWindowDisplayAffinity(WindowHandle);
		if (Result != EProtectionResult::Success)
		{
			FinalResult = EProtectionResult::PartialSuccess;
		}
	}

	ProtectedWindows.erase(It);
	Log("[%s] Removed Internal Protection FinalResult=%d", CLASS_NAME_TAG, FinalResult);
	return FinalResult;
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::ApplyProtectionToProcess(uint32_t ProcessId, uint32_t Methods)
{
	std::vector<WindowInfo> ProcessWindows = EnumerateProcessWindows(ProcessId);
	if (ProcessWindows.empty())
	{
		Log("[%s] No windows found for PID=%d", CLASS_NAME_TAG, ProcessId);
		return EProtectionResult::InvalidWindow;
	}

	EProtectionResult FinalResult = EProtectionResult::Success;
	uint32_t SuccessCount = 0;
	uint32_t TotalWindows = static_cast<uint32_t>(ProcessWindows.size());
	uint32_t EligibleWindows = 0;

	Log("[%s] Found %d total windows for PID=%d", CLASS_NAME_TAG, TotalWindows, ProcessId);

	std::sort(ProcessWindows.begin(), ProcessWindows.end(), [](const WindowInfo& a, const WindowInfo& b) {

		if (a.ClassName == "TCustomForm" && b.ClassName != "TCustomForm") return true;
		if (a.ClassName != "TCustomForm" && b.ClassName == "TCustomForm") return false;
		
		if (!a.WindowTitle.empty() && b.WindowTitle.empty()) return true;
		if (a.WindowTitle.empty() && !b.WindowTitle.empty()) return false;
		
		if (a.bIsTopLevel && !b.bIsTopLevel) return true;
		if (!a.bIsTopLevel && b.bIsTopLevel) return false;
		
		return false;
	});

	for (const WindowInfo& WinInfo : ProcessWindows)
	{
		bool bShouldProtect = ShouldProtectWindow(WinInfo);
		
		Log("[%s] Window evaluation: HWND=0x%p, Title='%s', Class='%s', ShouldProtect=%s", 
			CLASS_NAME_TAG, 
			WinInfo.WindowHandle, 
			WinInfo.WindowTitle.c_str(), 
			WinInfo.ClassName.c_str(),
			bShouldProtect ? "true" : "false");
		
		if (bShouldProtect)
		{
			EligibleWindows++;
			
			if (IsWindowProtected(WinInfo.WindowHandle))
			{
				Log("[%s] Window is already protected: HWND=0x%p", CLASS_NAME_TAG, WinInfo.WindowHandle);
				SuccessCount++;
				continue;
			}
			
			EProtectionResult Result = ApplyProtection(WinInfo.WindowHandle, Methods);
			
			Log("[%s] Protection attempt result: HWND=0x%p, Result=%d", 
				CLASS_NAME_TAG, WinInfo.WindowHandle, static_cast<int>(Result));
			
			if (Result == EProtectionResult::Success || Result == EProtectionResult::AlreadyProtected)
			{
				SuccessCount++;
			}
		}
	}

	if (SuccessCount == 0 && EligibleWindows > 0)
	{
		FinalResult = EProtectionResult::Failed;
	}
	else if (SuccessCount < EligibleWindows)
	{
		FinalResult = EProtectionResult::PartialSuccess;
	}

	Log("[%s] Applied Protection to %d/%d eligible windows (%d total) for PID=%d. FinalResult=%d", 
		CLASS_NAME_TAG, SuccessCount, EligibleWindows, TotalWindows, ProcessId, static_cast<int>(FinalResult));
	return FinalResult;
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::ApplyProtectionToCurrentProcess(uint32_t Methods)
{
	uint32_t CurrentProcessId = GetCurrentProcessId();
	return ApplyProtectionToProcess(CurrentProcessId, Methods);
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::RemoveProtectionFromProcess(uint32_t ProcessId)
{
	std::vector<WindowInfo> ProcessWindows = EnumerateProcessWindows(ProcessId);
	if (ProcessWindows.empty())
	{
		return EProtectionResult::Success;
	}

	EProtectionResult FinalResult = EProtectionResult::Success;
	uint32_t SuccessCount = 0;
	uint32_t TotalWindows = static_cast<uint32_t>(ProcessWindows.size());

	for (const WindowInfo& WinInfo : ProcessWindows)
	{
		EProtectionResult Result = RemoveProtection(WinInfo.WindowHandle);
		if (Result == EProtectionResult::Success)
		{
			SuccessCount++;
		}
	}

	if (SuccessCount < TotalWindows)
	{
		FinalResult = EProtectionResult::PartialSuccess;
	}

	Log("[%s] Removed Protection PID=%d FinalResult=%d", CLASS_NAME_TAG, ProcessId, FinalResult);
	return FinalResult;
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::ApplySetWindowDisplayAffinity(HWND WindowHandle)
{
	BOOL bCompositionEnabled = FALSE;
	HRESULT hr = DwmIsCompositionEnabled(&bCompositionEnabled);
	if (FAILED(hr) || !bCompositionEnabled)
	{
		Log("[%s] DWM composition is not enabled, SetWindowDisplayAffinity will not work", CLASS_NAME_TAG);
		return EProtectionResult::NotSupported;
	}

	if (!SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE))
	{
		if (!SetWindowDisplayAffinity(WindowHandle, WDA_MONITOR))
		{
			DWORD LastError = GetLastError();
			Log("[%s] SetWindowDisplayAffinity failed with error: %lu", CLASS_NAME_TAG, LastError);

			switch (LastError)
			{
			case ERROR_ACCESS_DENIED:
				return EProtectionResult::InsufficientPrivileges;
			case ERROR_INVALID_WINDOW_HANDLE:
				return EProtectionResult::InvalidWindow;
			default:
				return EProtectionResult::Failed;
			}
		}
	}
	return EProtectionResult::Success;
}

AntiScreenCapture::EProtectionResult AntiScreenCapture::RemoveSetWindowDisplayAffinity(HWND WindowHandle)
{
	if (!SetWindowDisplayAffinity(WindowHandle, WDA_NONE))
	{
		DWORD LastError = GetLastError();
		Log("[%s] Failed to remove SetWindowDisplayAffinity, error: %lu", CLASS_NAME_TAG, LastError);
		return EProtectionResult::Failed;
	}

	return EProtectionResult::Success;
}

bool AntiScreenCapture::IsWindowProtected(HWND WindowHandle) const
{
	std::lock_guard<std::mutex> Lock(ProtectedWindowsMutex);
	auto It = ProtectedWindows.find(WindowHandle);
	return It != ProtectedWindows.end() && It->second.bIsActive;
}

AntiScreenCapture::ProtectionStatus AntiScreenCapture::GetProtectionStatus(HWND WindowHandle) const
{
	std::lock_guard<std::mutex> Lock(ProtectedWindowsMutex);
	auto It = ProtectedWindows.find(WindowHandle);
	if (It != ProtectedWindows.end())
	{
		return It->second;
	}

	ProtectionStatus DefaultStatus;
	DefaultStatus.WindowHandle = WindowHandle;
	return DefaultStatus;
}

std::vector<AntiScreenCapture::ProtectionStatus> AntiScreenCapture::GetAllProtectedWindows() const
{
	std::lock_guard<std::mutex> Lock(ProtectedWindowsMutex);
	std::vector<ProtectionStatus> Result;
	Result.reserve(ProtectedWindows.size());

	for (const auto& Pair : ProtectedWindows)
	{
		if (Pair.second.bIsActive)
		{
			Result.push_back(Pair.second);
		}
	}

	return Result;
}

std::vector<AntiScreenCapture::WindowInfo> AntiScreenCapture::EnumerateProcessWindows(uint32_t ProcessId) const
{
	std::vector<WindowInfo> ProcessWindows;

	struct EnumData
	{
		uint32_t TargetProcessId;
		std::vector<WindowInfo>* WindowList;
		const AntiScreenCapture* Instance;
	};

	EnumData Data = { ProcessId, &ProcessWindows, this };

	EnumWindows(EnumerateWindowsCallback, reinterpret_cast<LPARAM>(&Data));
	return ProcessWindows;
}

BOOL CALLBACK AntiScreenCapture::EnumerateWindowsCallback(HWND WindowHandle, LPARAM lParam)
{
	auto* Data = reinterpret_cast<EnumData*>(lParam);

	DWORD WindowProcessId;
	GetWindowThreadProcessId(WindowHandle, &WindowProcessId);

	if (WindowProcessId == Data->TargetProcessId)
	{
		WindowInfo WinInfo = Data->Instance->GetWindowInfo(WindowHandle);
		Data->WindowList->push_back(WinInfo);
		Log(
			"[%s] Stored Window: HWND=0x%p, Title=\"%s\", Class=\"%s\", Rect=[%ld,%ld,%ld,%ld], PID=%u, Visible=%s, TopLevel=%s",
			CLASS_NAME_TAG,
			WinInfo.WindowHandle,
			WinInfo.WindowTitle.c_str(),
			WinInfo.ClassName.c_str(),
			WinInfo.WindowRect.left, WinInfo.WindowRect.top, WinInfo.WindowRect.right, WinInfo.WindowRect.bottom,
			WinInfo.ProcessId,
			WinInfo.bIsVisible ? "true" : "false",
			WinInfo.bIsTopLevel ? "true" : "false"
		);
	}

	return TRUE;
}

AntiScreenCapture::WindowInfo AntiScreenCapture::GetWindowInfo(HWND WindowHandle) const
{
	WindowInfo WinInfo;
	WinInfo.WindowHandle = WindowHandle;

	char WindowTitle[256] = { 0 };
	if (GetWindowTextA(WindowHandle, WindowTitle, sizeof(WindowTitle) - 1) > 0)
	{
		WinInfo.WindowTitle = WindowTitle;
	}

	char ClassName[256] = { 0 };
	if (GetClassNameA(WindowHandle, ClassName, sizeof(ClassName) - 1) > 0)
	{
		WinInfo.ClassName = ClassName;
	}

	GetWindowRect(WindowHandle, &WinInfo.WindowRect);

	GetWindowThreadProcessId(WindowHandle, reinterpret_cast<LPDWORD>(&WinInfo.ProcessId));

	WinInfo.bIsVisible = IsWindowVisible(WindowHandle) != FALSE;

	WinInfo.bIsTopLevel = (GetWindow(WindowHandle, GW_OWNER) == nullptr) &&
		(GetParent(WindowHandle) == nullptr || GetParent(WindowHandle) == GetDesktopWindow());

	WinInfo.Style = GetWindowLong(WindowHandle, GWL_STYLE);
	WinInfo.ExtendedStyle = GetWindowLong(WindowHandle, GWL_EXSTYLE);

	return WinInfo;
}

bool AntiScreenCapture::ValidateWindowHandle(HWND WindowHandle) const
{
	return WindowHandle != nullptr && IsWindow(WindowHandle) != FALSE;
}

bool AntiScreenCapture::ShouldProtectWindow(const WindowInfo& WindowInfo) const
{
	if (!WindowInfo.WindowHandle)
	{
		Log("[%s] Window handle is invalid", CLASS_NAME_TAG);
		return false;
	}

	if (WindowInfo.Style & WS_CHILD)
	{
		Log("[%s] Determined window is child", CLASS_NAME_TAG);
		return false;
	}

	if (WindowInfo.ExtendedStyle & WS_EX_TOOLWINDOW)
	{
		Log("[%s] Determined window is tool window", CLASS_NAME_TAG);
		return false;
	}

	if (WindowInfo.ClassName == TOOLTIPS_CLASSA || WindowInfo.ClassName == "tooltips_class32")
	{
		Log("[%s] Determined window is tooltip", CLASS_NAME_TAG);
		return false;
	}

	if (WindowInfo.ClassName == "ComboLBox" || 
		WindowInfo.ClassName == "IME" || 
		WindowInfo.ClassName == "WorkerW" ||
		WindowInfo.ClassName == "CicMarshalWndClass" ||
		WindowInfo.ClassName == "UserAdapterWindowClass" ||
		WindowInfo.ClassName == "MSCTFIME UI" ||
		WindowInfo.ClassName == "OleMainThreadWndClass")
	{
		Log("[%s] Determined window is system/utility window: %s", CLASS_NAME_TAG, WindowInfo.ClassName.c_str());
		return false;
	}

	if (WindowInfo.ClassName == "TCustomForm")
	{
		Log("[%s] Protecting TCustomForm window with title: %s", CLASS_NAME_TAG, WindowInfo.WindowTitle.c_str());
		return true;
	}

	if (WindowInfo.Style & WS_CAPTION)
	{
		Log("[%s] Protecting window with caption: %s", CLASS_NAME_TAG, WindowInfo.WindowTitle.c_str());
		return true;
	}

	if (WindowInfo.Style & WS_POPUP)
	{
		if (!WindowInfo.WindowTitle.empty() && !IsTransientPopup(WindowInfo))
		{
			Log("[%s] Protecting pop-up window: %s", CLASS_NAME_TAG, WindowInfo.WindowTitle.c_str());
			return true;
		}
	}

	if (WindowInfo.bIsTopLevel && !WindowInfo.WindowTitle.empty())
	{
		Log("[%s] Protecting top-level window: %s", CLASS_NAME_TAG, WindowInfo.WindowTitle.c_str());
		return true;
	}

	Log("[%s] Window does not meet protection criteria: HWND=0x%p, Title='%s', Class='%s', Style=0x%08X, ExStyle=0x%08X, TopLevel=%s",
		CLASS_NAME_TAG,
		WindowInfo.WindowHandle,
		WindowInfo.WindowTitle.c_str(),
		WindowInfo.ClassName.c_str(),
		WindowInfo.Style,
		WindowInfo.ExtendedStyle,
		WindowInfo.bIsTopLevel ? "true" : "false");

	return false;
}

bool AntiScreenCapture::IsTransientPopup(const WindowInfo& WindowInfo) const
{
	if (WindowInfo.ClassName == TOOLTIPS_CLASSA || WindowInfo.ClassName == "tooltips_class32")
	{
		Log("[%s] Determined class name matches tooltips", CLASS_NAME_TAG);
		return true;
	}

	if (WindowInfo.ClassName == "#32768")
	{
		Log("[%s] Determined class name matches menu", CLASS_NAME_TAG);
		return false;
	}

	if (WindowInfo.ClassName == "SysShadow")
	{
		Log("[%s] Determined class name matches shadow window", CLASS_NAME_TAG);
		return true;
	}

	if ((WindowInfo.Style & WS_POPUP) && (WindowInfo.ExtendedStyle & WS_EX_TOOLWINDOW))
	{
		HWND owner = GetWindow(WindowInfo.WindowHandle, GW_OWNER);
		if (owner == nullptr)
		{
			Log("[%s] Determined style is pop-up and tool window without owner", CLASS_NAME_TAG);
			return true;
		}
	}

	if ((WindowInfo.Style & WS_POPUP) && WindowInfo.WindowTitle.empty())
	{
		int width = WindowInfo.WindowRect.right - WindowInfo.WindowRect.left;
		int height = WindowInfo.WindowRect.bottom - WindowInfo.WindowRect.top;
		
		if (width <= 0 || height <= 0 || (width < 100 && height < 100))
		{
			Log("[%s] Determined small pop-up window without title (likely transient)", CLASS_NAME_TAG);
			return true;
		}
	}

	if (WindowInfo.ExtendedStyle & WS_EX_NOACTIVATE)
	{
		Log("[%s] Determined window has WS_EX_NOACTIVATE style", CLASS_NAME_TAG);
		return true;
	}

	if ((WindowInfo.ExtendedStyle & WS_EX_LAYERED) && WindowInfo.WindowTitle.empty())
	{
		Log("[%s] Determined layered window without title", CLASS_NAME_TAG);
		return true;
	}

	return false;
}

bool AntiScreenCapture::StartWindowEventHook(uint32_t ProcessId)
{
	if (bIsHookActive.load())
	{
		Log("[%s] Window event hook is already active", CLASS_NAME_TAG);
		return true;
	}

	if (!bIsInitialized)
	{
		Log("[%s] Cannot start hook - system not initialized", CLASS_NAME_TAG);
		return false;
	}

	Log("[%s] Starting window event hook for PID=%d", CLASS_NAME_TAG, ProcessId);

	HookInstance = this;
	HookTargetProcessId = ProcessId;

	bHookMessageThreadActive.store(true);
	HookMessageThread = std::make_unique<std::thread>([this]() {
		HookMessageThreadFunction();
		});

	
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	if (!MessageWindow)
	{
		Log("[%s] Failed to create message window for hook", CLASS_NAME_TAG);
		bHookMessageThreadActive.store(false);
		if (HookMessageThread && HookMessageThread->joinable())
		{
			HookMessageThread->join();
		}
		HookMessageThread.reset();
		HookInstance = nullptr;
		return false;
	}

	WindowEventHook = SetWinEventHook(
		EVENT_OBJECT_CREATE,
		EVENT_OBJECT_DESTROY,
		nullptr,
		WindowEventHookCallback,
		0,
		0,
		WINEVENT_OUTOFCONTEXT
	);

	if (!WindowEventHook)
	{
		DWORD LastError = GetLastError();
		Log("[%s] Failed to set window event hook, error: %lu", CLASS_NAME_TAG, LastError);
		bHookMessageThreadActive.store(false);
		if (MessageWindow)
		{
			PostMessage(MessageWindow, WM_QUIT, 0, 0);
		}
		if (HookMessageThread && HookMessageThread->joinable())
		{
			HookMessageThread->join();
		}
		HookMessageThread.reset();
		HookInstance = nullptr;
		return false;
	}

	bIsHookActive.store(true);
	Log("[%s] Window event hook started successfully for PID=%d", CLASS_NAME_TAG, ProcessId);
	return true;
}

void AntiScreenCapture::StopWindowEventHook()
{
	if (!bIsHookActive.load())
	{
		return;
	}

	bIsHookActive.store(false);

	HookInstance = nullptr;

	if (WindowEventHook)
	{
		UnhookWinEvent(WindowEventHook);
		WindowEventHook = nullptr;
	}

	bHookMessageThreadActive.store(false);
	if (MessageWindow)
	{
		PostMessage(MessageWindow, WM_QUIT, 0, 0);
	}

	if (HookMessageThread && HookMessageThread->joinable())
	{
		try
		{
			HookMessageThread->join();
		}
		catch (const std::exception& Exception)
		{
			Log("[%s] Exception while joining hook message thread: %s", CLASS_NAME_TAG, Exception.what());
		}
		HookMessageThread.reset();
	}

	MessageWindow = nullptr;
	HookTargetProcessId = 0;

	Log("[AntiScreenCapture] Window event hook stopped");
}

bool AntiScreenCapture::IsWindowEventHookActive() const
{
	return bIsHookActive.load();
}

void CALLBACK AntiScreenCapture::WindowEventHookCallback(
	HWINEVENTHOOK hWinEventHook,
	DWORD event,
	HWND hwnd,
	LONG idObject,
	LONG idChild,
	DWORD dwEventThread,
	DWORD dwmsEventTime)
{
	if (!HookInstance || !hwnd || !HookInstance->MessageWindow)
	{
		return;
	}

	if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
	{
		return;
	}

	if (event == EVENT_OBJECT_DESTROY)
	{
		PostMessage(HookInstance->MessageWindow, WM_WINDOW_EVENT_HOOK_DESTROY, 0, reinterpret_cast<LPARAM>(hwnd));
		return;
	}

	if (event == EVENT_OBJECT_CREATE || event == EVENT_OBJECT_SHOW)
	{
		PostMessage(HookInstance->MessageWindow, WM_WINDOW_EVENT_HOOK, event, reinterpret_cast<LPARAM>(hwnd));
		return;
	}
}

bool AntiScreenCapture::CreateMessageWindow()
{
	static const wchar_t* WindowClassName = L"FacelessMessageWindow";
	static const wchar_t* WindowName = L"FacelessMessageWindow";

	Log("[%s] Creating message window for hook events", CLASS_NAME_TAG);

	WNDCLASSW wc = {};
	wc.lpfnWndProc = MessageWindowProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = WindowClassName;

	if (!GetClassInfoW(wc.hInstance, WindowClassName, &wc))
	{
		if (!RegisterClassW(&wc))
		{
			DWORD LastError = GetLastError();
			Log("[%s] Failed to register message window class, error: %lu", CLASS_NAME_TAG, LastError);
			return false;
		}
		Log("[%s] Registered message window class", CLASS_NAME_TAG);
	}

	MessageWindow = CreateWindowExW(
		0,
		WindowClassName,
		WindowName,
		0,
		0, 0, 0, 0,
		HWND_MESSAGE,
		nullptr,
		GetModuleHandle(nullptr),
		this
	);

	if (!MessageWindow)
	{
		DWORD LastError = GetLastError();
		Log("[%s] Failed to create message window, error: %lu", CLASS_NAME_TAG, LastError);
		return false;
	}
	Log("[%s] Message window created successfully: HWND=0x%p", CLASS_NAME_TAG, MessageWindow);
	return true;
}

void AntiScreenCapture::DestroyMessageWindow()
{
	if (MessageWindow)
	{
		DestroyWindow(MessageWindow);
		MessageWindow = nullptr;
	}
	Log("[%s] Message window destroyed", CLASS_NAME_TAG);
}

LRESULT CALLBACK AntiScreenCapture::MessageWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CREATE)
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreate->lpCreateParams));
		return 0;
	}

	AntiScreenCapture* pInstance = reinterpret_cast<AntiScreenCapture*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (uMsg == WM_WINDOW_EVENT_HOOK && pInstance)
	{
		DWORD event = static_cast<DWORD>(wParam);
		HWND windowHandle = reinterpret_cast<HWND>(lParam);

		if (!pInstance->ValidateWindowHandle(windowHandle))
		{
			Log("[%s] Invalid window handle in hook: HWND=0x%p", CLASS_NAME_TAG, windowHandle);
			return 0;
		}

		WindowInfo WinInfo;
		try
		{
			WinInfo = pInstance->GetWindowInfo(windowHandle);

			Log("[%s] Hooked Event 0x%X for window HWND=0x%p, Title='%s', Class='%s', PID=%d",
				CLASS_NAME_TAG,
				event,
				windowHandle,
				WinInfo.WindowTitle.c_str(),
				WinInfo.ClassName.c_str(),
				WinInfo.ProcessId);

			if (pInstance->HookTargetProcessId != 0 && WinInfo.ProcessId != pInstance->HookTargetProcessId)
			{
				Log("[%s] Window PID %d does not match target PID %d, ignoring", 
					CLASS_NAME_TAG, WinInfo.ProcessId, pInstance->HookTargetProcessId);
				return 0;
			}

			if (pInstance->IsTransientPopup(WinInfo))
			{
				Log("[%s] Detected transient pop-up window, hiding: HWND=0x%p, Title='%s', Class='%s'",
					CLASS_NAME_TAG,
					windowHandle,
					WinInfo.WindowTitle.c_str(),
					WinInfo.ClassName.c_str());
				ShowWindow(windowHandle, SW_HIDE);
				return 0;
			}

			bool bShouldProtect = pInstance->ShouldProtectWindow(WinInfo);
			Log("[%s] Window protection evaluation: HWND=0x%p, ShouldProtect=%s", 
				CLASS_NAME_TAG, windowHandle, bShouldProtect ? "true" : "false");

			if (!bShouldProtect)
			{
				return 0;
			}

			if (pInstance->IsWindowProtected(windowHandle))
			{
				Log("[%s] Window is already protected: HWND=0x%p", CLASS_NAME_TAG, windowHandle);
				return 0;
			}
			
			uint32_t ProtectionMethods = static_cast<uint32_t>(EProtectionMethod::DisplayAffinity);
			EProtectionResult Result = pInstance->ApplyProtection(windowHandle, ProtectionMethods);
			
			Log("[%s] Applied protection to new window: HWND=0x%p, Title='%s', Class='%s', Event=0x%X, Result=%d",
				CLASS_NAME_TAG,
				windowHandle,
				WinInfo.WindowTitle.c_str(),
				WinInfo.ClassName.c_str(),
				event,
				static_cast<int>(Result));

			return 0;
		}
		catch (const std::exception& Exception)
		{
			Log("[%s] Exception while processing window hook event: %s", CLASS_NAME_TAG, Exception.what());
			return 0;
		}
	}
	if (uMsg == WM_WINDOW_EVENT_HOOK_DESTROY && pInstance)
	{
		HWND windowHandle = reinterpret_cast<HWND>(lParam);
		try
		{
			std::lock_guard<std::mutex> Lock(pInstance->ProtectedWindowsMutex);
			if (pInstance->ProtectedWindows.count(windowHandle))
			{
				pInstance->RemoveProtectionInternal(windowHandle);
			}
			Log("[%s] Attempted to remove protection on HWND=0x%p",
				CLASS_NAME_TAG,
				windowHandle);
			return 0;
		}
		catch (const std::exception& Exception)
		{
			Log("[%s] Exception processing window destroy event: %s", CLASS_NAME_TAG, Exception.what());
			return 0;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void AntiScreenCapture::HookMessageThreadFunction()
{
	try
	{
		if (!CreateMessageWindow())
		{
			return;
		}

		MSG msg;
		while (bHookMessageThreadActive.load() && GetMessage(&msg, MessageWindow, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		DestroyMessageWindow();
	}
	catch (const std::exception& Exception)
	{
		Log("[%s] Exception in hook message thread: %s", CLASS_NAME_TAG, Exception.what());
	}
}