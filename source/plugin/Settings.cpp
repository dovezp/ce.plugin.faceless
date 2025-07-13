#include "Settings.h"
#include "PluginDetails.h"
#include "Logger.h"
#include <fstream>
#include <filesystem>
#include <sstream>

#define CLASS_NAME_TAG __FILE__

Settings& Settings::GetInstance()
{
	static Settings Instance;
	return Instance;
}

Settings::Settings()
	: bIsInitialized(false)
	, bIsValid(false)
{
	AntiScreenCaptureSettings = AntiScreenCaptureConfig{};
}

bool Settings::Initialize()
{
	if (!LoadFromFile())
	{
		Log("[%s] Failed to load configuration from file", CLASS_NAME_TAG);
	}

	bIsInitialized = true;
	bIsValid = true;

	return true;
}

bool Settings::LoadFromFile()
{
	try
	{
		INIReader iniReader("faceless.ini");

		if (iniReader.ParseError() != 0)
		{
			Log("[%s] INI parsing error: %d", CLASS_NAME_TAG, iniReader.ParseError());
			return false;
		}

		AntiScreenCaptureSettings.bEnabled = iniReader.GetBoolean("Settings", "Enabled", true);


		Log("[%s] Configuration loaded successfully: Enabled=%s",
			CLASS_NAME_TAG,
			AntiScreenCaptureSettings.bEnabled ? "true" : "false");
		return true;
	}
	catch (const std::exception& Exception)
	{
		Log("[%s] Exception during configuration loading: %s", CLASS_NAME_TAG, Exception.what());
		return false;
	}
}

const Settings::AntiScreenCaptureConfig& Settings::GetAntiScreenCaptureConfig() const
{
	std::lock_guard<std::mutex> Lock(SettingsMutex);
	return AntiScreenCaptureSettings;
}

void Settings::SetAntiScreenCaptureEnabled(bool bEnabled)
{
	std::lock_guard<std::mutex> Lock(SettingsMutex);

	if (AntiScreenCaptureSettings.bEnabled != bEnabled)
	{
		AntiScreenCaptureSettings.bEnabled = bEnabled;
		Log("[%s] AntiScreenCapture enabled state changed to: %s", CLASS_NAME_TAG, bEnabled ? "true" : "false");
	}
}

bool Settings::IsValid() const
{
	std::lock_guard<std::mutex> Lock(SettingsMutex);
	return bIsValid;
}