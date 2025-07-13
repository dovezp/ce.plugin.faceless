#pragma once

#include "Dependencies.h"
#include "Settings.h"
#include <Windows.h>
#include <dwmapi.h>
#include <WinUser.h>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

class AntiScreenCapture
{
public:
	/**
	 * @enum EProtectionMethod
	 * @brief Available protection methods for anti-screen capture
	 */
	enum class EProtectionMethod : uint32_t
	{
		None = 0x00000000,
		DisplayAffinity = 0x00000001,
		All = 0x00000001
	};

	/**
	 * @enum EProtectionResult
	 * @brief Result codes for protection operations
	 */
	enum class EProtectionResult : uint32_t
	{
		Success = 0,
		Failed = 1,
		NotSupported = 2,
		InvalidWindow = 3,
		InsufficientPrivileges = 4,
		PartialSuccess = 5,
		AlreadyProtected = 6
	};

	/**
	 * @struct ProtectionStatus
	 * @brief Status information for a protected window
	 */
	struct ProtectionStatus
	{
		// Window handle
		HWND WindowHandle = nullptr;
		// Bitmask of applied protection methods
		uint32_t AppliedMethods = 0;
		// Last operation result
		EProtectionResult LastResult = EProtectionResult::Failed;
		// Last update timestamp
		std::chrono::steady_clock::time_point LastUpdate;
		// Number of retry attempts
		uint32_t RetryCount = 0;
		// Whether protection is currently active
		bool bIsActive = false;
		// Associated process name
		std::string ProcessName;
		// Associated process ID
		uint32_t ProcessId = 0;
	};

	/**
	 * @struct WindowInfo
	 * @brief Information about a window for protection
	 */
	struct WindowInfo
	{
		// Window handle
		HWND WindowHandle = nullptr;
		// Window title
		std::string WindowTitle;
		// Window class name
		std::string ClassName;
		// Window rectangle
		RECT WindowRect = {};
		// Owner process ID
		uint32_t ProcessId = 0;
		// Window visibility state
		bool bIsVisible = false;
		// Whether it's a top-level window
		bool bIsTopLevel = false;
		// Window style
		LONG Style = 0;
		// Extended window style
		LONG ExtendedStyle = 0;
	};

public:
	AntiScreenCapture();

	~AntiScreenCapture();

	/**
	 * @brief Initialize the anti-screen capture system
	 * @return true if initialization succeeded, false otherwise
	 */
	bool Initialize();

	/**
	 * @brief Shutdown the anti-screen capture system
	 */
	void Shutdown();

	/**
	 * @brief Apply protection to a specific window
	 * @param WindowHandle Handle to the window to protect
	 * @param Methods Bitmask of protection methods to apply
	 * @return Result of the protection operation
	 */
	EProtectionResult ApplyProtection(HWND WindowHandle, uint32_t Methods = static_cast<uint32_t>(EProtectionMethod::All));

	/**
	 * @brief Remove protection from a specific window
	 * @param WindowHandle Handle to the window to unprotect
	 * @return Result of the unprotection operation
	 */
	EProtectionResult RemoveProtection(HWND WindowHandle);

	/**
	 * @brief Apply protection to all windows of a specific process
	 * @param ProcessId Process ID to protect
	 * @param Methods Bitmask of protection methods to apply
	 * @return Result of the protection operation
	 */
	EProtectionResult ApplyProtectionToProcess(uint32_t ProcessId, uint32_t Methods = static_cast<uint32_t>(EProtectionMethod::All));

	/**
	 * @brief Apply protection to the current process (Cheat Engine)
	 * @param Methods Bitmask of protection methods to apply
	 * @return Result of the protection operation
	 */
	EProtectionResult ApplyProtectionToCurrentProcess(uint32_t Methods = static_cast<uint32_t>(EProtectionMethod::All));

	/**
	 * @brief Remove protection from all windows of a specific process
	 * @param ProcessId Process ID to unprotect
	 * @return Result of the unprotection operation
	 */
	EProtectionResult RemoveProtectionFromProcess(uint32_t ProcessId);

	/**
	 * @brief Check if a window is currently protected
	 * @param WindowHandle Window handle to check
	 * @return true if the window is protected, false otherwise
	 */
	bool IsWindowProtected(HWND WindowHandle) const;

	/**
	 * @brief Get protection status for a window
	 * @param WindowHandle Window handle to query
	 * @return Protection status information
	 */
	ProtectionStatus GetProtectionStatus(HWND WindowHandle) const;

	/**
	 * @brief Get all currently protected windows
	 * @return Vector of protection status for all protected windows
	 */
	std::vector<ProtectionStatus> GetAllProtectedWindows() const;

	/**
	 * @brief Enumerate all windows for a specific process
	 * @param ProcessId Process ID to enumerate windows for
	 * @return Vector of window information
	 */
	std::vector<WindowInfo> EnumerateProcessWindows(uint32_t ProcessId) const;

	/**
	 * @brief Check if a window should be protected based on filtering criteria
	 * @param WindowInfo Window information to evaluate
	 * @return true if window should be protected, false otherwise
	 */
	bool ShouldProtectWindow(const WindowInfo& WindowInfo) const;

	/**
	 * @brief Check if a window is a transient pop-up (tooltip, menu, etc.)
	 * @param WindowInfo Window information to evaluate
	 * @return true if window is a transient pop-up, false otherwise
	 */
	bool IsTransientPopup(const WindowInfo& WindowInfo) const;

	/**
	 * @brief Start hook-based window detection for immediate protection
	 * @param ProcessId Process ID to monitor for new windows (0 = all processes)
	 * @return true if hook started successfully, false otherwise
	 */
	bool StartWindowEventHook(uint32_t ProcessId = 0);

	/**
	 * @brief Stop hook-based window detection
	 */
	void StopWindowEventHook();

	/**
	 * @brief Check if window event hook is currently active
	 * @return true if hook is active, false otherwise
	 */
	bool IsWindowEventHookActive() const;

private:
	// Custom Windows messages for hook events
	static constexpr UINT WM_WINDOW_EVENT_HOOK = WM_USER + 1;
	static constexpr UINT WM_WINDOW_EVENT_HOOK_DESTROY = WM_USER + 2;

	/**
	 * @struct WindowEventMessage
	 * @brief Structure for window event messages
	 */
	struct WindowEventMessage
	{
		DWORD Event;
		HWND WindowHandle;
	};

private:
	/**
	 * @brief Apply SetWindowDisplayAffinity protection
	 * @param WindowHandle Window handle to protect
	 * @return Result of the operation
	 */
	EProtectionResult ApplySetWindowDisplayAffinity(HWND WindowHandle);

	/**
	 * @brief Remove SetWindowDisplayAffinity protection
	 * @param WindowHandle Window handle to unprotect
	 * @return Result of the operation
	 */
	EProtectionResult RemoveSetWindowDisplayAffinity(HWND WindowHandle);

	/**
	 * @brief Validate that a window handle is valid and accessible
	 * @param WindowHandle Window handle to validate
	 * @return true if the window is valid, false otherwise
	 */
	bool ValidateWindowHandle(HWND WindowHandle) const;

	/**
	 * @brief Get detailed information about a window
	 * @param WindowHandle Window handle to query
	 * @return Window information structure
	 */
	WindowInfo GetWindowInfo(HWND WindowHandle) const;

	/**
	 * @brief Window enumeration callback for process window enumeration
	 * @param WindowHandle Window handle
	 * @param lParam User data (pointer to vector<WindowInfo>)
	 * @return TRUE to continue enumeration, FALSE to stop
	 */
	static BOOL CALLBACK EnumerateWindowsCallback(HWND WindowHandle, LPARAM lParam);

	/**
	 * @brief Internal method to apply protection without acquiring mutex
	 * @note Caller must hold ProtectedWindowsMutex
	 * @param WindowHandle Handle to the window to protect
	 * @param Methods Bitmask of protection methods to apply
	 * @return Result of the protection operation
	 */
	EProtectionResult ApplyProtectionInternal(HWND WindowHandle, uint32_t Methods);

	/**
	 * @brief Internal method to remove protection without acquiring mutex
	 * @note Caller must hold ProtectedWindowsMutex
	 * @param WindowHandle Handle to the window to unprotect
	 * @return Result of the unprotection operation
	 */
	EProtectionResult RemoveProtectionInternal(HWND WindowHandle);

	/**
	 * @brief Window event hook callback function
	 * @param hWinEventHook Handle to the event hook
	 * @param event Event that occurred
	 * @param hwnd Window handle
	 * @param idObject Object identifier
	 * @param idChild Child identifier
	 * @param dwEventThread Thread that generated the event
	 * @param dwmsEventTime Time the event was generated
	 */
	static void CALLBACK WindowEventHookCallback(
		HWINEVENTHOOK hWinEventHook,
		DWORD event,
		HWND hwnd,
		LONG idObject,
		LONG idChild,
		DWORD dwEventThread,
		DWORD dwmsEventTime
	);

	/**
	 * @brief Create the hidden message window for receiving hook events
	 * @return true if window created successfully, false otherwise
	 */
	bool CreateMessageWindow();

	/**
	 * @brief Destroy the hidden message window
	 */
	void DestroyMessageWindow();

	/**
	 * @brief Window procedure for the hidden message window
	 * @param hwnd Window handle
	 * @param uMsg Message
	 * @param wParam wParam
	 * @param lParam lParam
	 * @return Message result
	 */
	static LRESULT CALLBACK MessageWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/**
	 * @brief Message loop thread function for hook events
	 */
	void HookMessageThreadFunction();

private:
	// Enumeration helper structure
	struct EnumData
	{
		uint32_t TargetProcessId;
		std::vector<WindowInfo>* WindowList;
		const AntiScreenCapture* Instance;
	};

	// Mutex for thread-safe access to protected windows
	mutable std::mutex ProtectedWindowsMutex;
	// Map of protected windows and their status
	std::map<HWND, ProtectionStatus> ProtectedWindows;

	// Window event hook handle
	HWINEVENTHOOK WindowEventHook;
	// Hook active flag
	std::atomic<bool> bIsHookActive;
	// Target process ID for hook
	uint32_t HookTargetProcessId;

	//  Hidden window for receiving hook messages
	HWND MessageWindow;
	// Thread running the message loop
	std::unique_ptr<std::thread> HookMessageThread;
	// Message thread active flag
	std::atomic<bool> bHookMessageThreadActive;
	static AntiScreenCapture* HookInstance;

	// Bitmask of supported protection methods
	uint32_t SupportedMethods;
	bool bIsInitialized;
};
