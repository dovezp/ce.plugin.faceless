#pragma once

#include "Dependencies.h"
#include <ini/INIReader.h>
#include <string>
#include <memory>
#include <mutex>

class Settings
{
public:
	/**
	 * @struct AntiScreenCaptureConfig
	 * @brief Configuration settings for anti-screen capture functionality
	 */
	struct AntiScreenCaptureConfig
	{
		// Enable/disable anti-screen capture
		bool bEnabled = true;
	};

public:
	/**
	 * @brief Get the singleton instance of Settings
	 * @return Reference to the Settings singleton
	 */
	static Settings& GetInstance();

	/**
	 * @brief Initialize settings from INI file
	 * @return true if initialization succeeded, false otherwise
	 */
	bool Initialize();

	/**
	 * @brief Get anti-screen capture configuration
	 * @return Reference to AntiScreenCaptureConfig
	 */
	const AntiScreenCaptureConfig& GetAntiScreenCaptureConfig() const;

	/**
	 * @brief Set anti-screen capture enabled state
	 * @param bEnabled Enable or disable anti-screen capture
	 */
	void SetAntiScreenCaptureEnabled(bool bEnabled);

	/**
	 * @brief Check if settings are valid and loaded
	 * @return true if settings are valid, false otherwise
	 */
	bool IsValid() const;

private:
	/**
	 * @brief Private constructor for singleton pattern
	 */
	Settings();
	~Settings() = default;

	/**
	 * @brief Deleted copy constructor
	 */
	Settings(const Settings&) = delete;

	/**
	 * @brief Deleted copy assignment operator
	 */
	Settings& operator=(const Settings&) = delete;

	/**
	 * @brief Load settings from INI file
	 * @return true if loading succeeded, false otherwise
	 */
	bool LoadFromFile();

private:
	// Anti-screen capture settings
	AntiScreenCaptureConfig AntiScreenCaptureSettings;
	// Thread safety mutex
	mutable std::mutex SettingsMutex;
	// Initialization state flag
	bool bIsInitialized;
	// Validity state flag
	bool bIsValid;
};
