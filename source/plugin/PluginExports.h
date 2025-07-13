#pragma once
#include "Dependencies.h"

#ifdef _WIN64
#pragma comment(linker, "/EXPORT:CEPlugin_GetVersion=?CEPlugin_GetVersion@@YAHPEAU_PluginVersion@@H@Z")
#pragma comment(linker, "/EXPORT:CEPlugin_InitializePlugin=?CEPlugin_InitializePlugin@@YAHPEAU_ExportedFunctions@@H@Z")
#pragma comment(linker, "/EXPORT:CEPlugin_DisablePlugin=?CEPlugin_DisablePlugin@@YAHXZ")
#else //x86
#pragma comment(linker, "/EXPORT:CEPlugin_GetVersion=?CEPlugin_GetVersion@@YGHPAU_PluginVersion@@H@Z")
#pragma comment(linker, "/EXPORT:CEPlugin_InitializePlugin=?CEPlugin_InitializePlugin@@YGHPAU_ExportedFunctions@@H@Z")
#pragma comment(linker, "/EXPORT:CEPlugin_DisablePlugin=?CEPlugin_DisablePlugin@@YGHXZ")
#endif

extern BOOL APIENTRY CEPlugin_GetVersion(PPluginVersion pPluginVersion, int SizePluginVersion);
extern BOOL APIENTRY CEPlugin_InitializePlugin(PExportedFunctions pExportedFunctions, int PluginId);
extern BOOL APIENTRY CEPlugin_DisablePlugin(void);
