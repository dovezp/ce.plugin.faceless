#pragma once

#include "Dependencies.h"
#include "IPlugin.h"
#include "Settings.h"
#include "AntiScreenCapture.h"
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

class Plugin : public IPlugin
{
public:
	/**
	 * @enum EPluginState
	 * @brief Current state of the plugin
	 */
	enum class EPluginState : uint32_t
	{
		Uninitialized = 0,
		Initializing = 1,
		Active = 2,
		Inactive = 3,
		Error = 4,
		Shutting_Down = 5
	};

public:
	Plugin();
	~Plugin() override;

	/**
	 * @brief Initialize the plugin with optional configuration file
	 * @param ConfigFilePath Path to configuration file (optional)
	 * @return true if initialization succeeded, false otherwise
	 */
	bool Initialize();

	/**
	 * @brief Shutdown the plugin and clean up all resources
	 */
	void Shutdown();

	/**
	 * @brief Start anti-screen capture protection
	 * @return true if protection started successfully, false otherwise
	 */
	bool StartProtection();

	/**
	 * @brief Stop anti-screen capture protection
	 * @return true if protection stopped successfully, false otherwise
	 */
	bool StopProtection();

	/**
	 * @brief Check if protection is currently active
	 * @return true if protection is active, false otherwise
	 */
	bool IsProtectionActive() const;

	/**
	 * @brief Get current plugin state
	 * @return Current plugin state
	 */
	EPluginState GetPluginState() const;

	/**
	 * @brief Enable/disable automatic protection for new Cheat Engine windows
	 * @param bEnabled Enable or disable auto-protection
	 */
	void SetAutoProtectionEnabled(bool bEnabled);

	/**
	 * @brief Check if auto-protection is enabled
	 * @return true if auto-protection is enabled, false otherwise
	 */
	bool IsAutoProtectionEnabled() const;

	/**
	 * @brief Manually protect the current Cheat Engine process
	 * @return true if protection was applied successfully, false otherwise
	 */
	bool ProtectCurrentProcess();

	/**
	 * @brief Remove protection from the current Cheat Engine process
	 * @return true if protection was removed successfully, false otherwise
	 */
	bool UnprotectCurrentProcess();

	/**
	 * @brief Get plugin version information
	 * @return Version string
	 */
	static std::string GetVersion();

	/**
	 * @brief Get plugin name
	 * @return Plugin name string
	 */
	static std::string GetName();

	/**
	 * @brief Get plugin description
	 * @return Plugin description string
	 */
	static std::string GetDescription();

	bool OnPluginLoad() override;
	void OnPluginUnload() override;
	void OnProcessAttach(uint32_t ProcessId) override;
	void OnProcessDetach(uint32_t ProcessId) override;

private:
	/**
	 * @brief Internal initialization of all subsystems
	 * @return true if all subsystems initialized successfully, false otherwise
	 */
	bool InitializeSubsystems();

	/**
	 * @brief Internal shutdown of all subsystems
	 */
	void ShutdownSubsystems();

	/**
	 * @brief Handle configuration changes
	 */
	void OnConfigurationChanged();

	/**
	 * @brief Validate plugin state transition
	 * @param NewState New state to transition to
	 * @return true if transition is valid, false otherwise
	 */
	bool IsValidStateTransition(EPluginState NewState) const;

	/**
	 * @brief Set plugin state with validation
	 * @param NewState New state to set
	 * @return true if state was set successfully, false otherwise
	 */
	bool SetPluginState(EPluginState NewState);

private:
	// Anti-screen capture system
	std::unique_ptr<AntiScreenCapture> AntiScreenCaptureSystem;

	// Mutex for thread-safe state access
	mutable std::mutex StateMutex;
	std::atomic<EPluginState> CurrentState;
	std::atomic<bool> bIsProtectionActive;
	std::atomic<bool> bIsAutoProtectionEnabled;
	std::atomic<bool> bIsInitialized;
};
