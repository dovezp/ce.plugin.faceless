#include "PluginExports.h"
#include "PluginDetails.h"
#include "Plugin.h"
#include "Logger.h"
#include <memory>

static std::unique_ptr<Plugin> g_PluginInstance = nullptr;
static PExportedFunctions g_ExportedFunctions = nullptr;
static int g_PluginId = -1;

BOOL APIENTRY CEPlugin_GetVersion(PPluginVersion pPluginVersion, int SizePluginVersion)
{
	pPluginVersion->version = CESDK_VERSION;
	pPluginVersion->pluginname = PLUGIN_DESCRIPTION;

	return TRUE;
}

BOOL APIENTRY CEPlugin_InitializePlugin(PExportedFunctions pExportedFunctions, int PluginId)
{
	if (!pExportedFunctions)
	{
		return FALSE;
	}

	g_ExportedFunctions = pExportedFunctions;
	g_PluginId = PluginId;

	try
	{
		g_PluginInstance = std::make_unique<Plugin>();
		if (!g_PluginInstance->Initialize())
		{
			g_PluginInstance.reset();
			return FALSE;
		}
		if (!g_PluginInstance->OnPluginLoad())
		{
			return FALSE;
		}

		Log("[%s] Initialized on %s", PLUGIN_NAME, PLUGIN_HOST);
		return TRUE;
	}
	catch (const std::exception& Exception)
	{
		Log("[%s] Encountered an exception on initialization: %s", PLUGIN_NAME, Exception.what());
		if (g_PluginInstance)
		{
			g_PluginInstance.reset();
		}

		return FALSE;
	}
}

BOOL APIENTRY CEPlugin_DisablePlugin(void)
{
	try
	{
		if (g_PluginInstance)
		{
			g_PluginInstance->OnPluginUnload();

			g_PluginInstance->Shutdown();
			g_PluginInstance.reset();
		}

		g_ExportedFunctions = nullptr;
		g_PluginId = -1;

		Log("[%s] Disabled on %s", PLUGIN_NAME, PLUGIN_HOST);
		return TRUE;
	}
	catch (const std::exception& Exception)
	{
		Log("[%s] Encountered an exception on disabling: %s", PLUGIN_NAME, Exception.what());
		if (g_PluginInstance)
		{
			g_PluginInstance.reset();
		}

		g_ExportedFunctions = nullptr;
		g_PluginId = -1;

		return FALSE;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hModule);
	UNREFERENCED_PARAMETER(lpvReserved);

	switch (ulReason)
	{
	default:
	case DLL_PROCESS_ATTACH:
	{
		Log("[%s] Attached to %s. Using Build: %s", PLUGIN_NAME, PLUGIN_HOST, __TIMESTAMP__);
		return TRUE;
	}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	{
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		Log("[%s] Detached from %s", PLUGIN_NAME, PLUGIN_HOST);
		break;
	}
	}
	return TRUE;
}