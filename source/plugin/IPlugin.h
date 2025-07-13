#pragma once

#include "Dependencies.h"

class IPlugin
{
public:
	virtual ~IPlugin() = default;

	/**
	 * @brief Called when the plugin is loaded by Cheat Engine
	 * @return true if plugin loaded successfully, false otherwise
	 */
	virtual bool OnPluginLoad() = 0;

	/**
	 * @brief Called when the plugin is unloaded by Cheat Engine
	 */
	virtual void OnPluginUnload() = 0;

	/**
	 * @brief Called when a process is attached in Cheat Engine
	 * @param ProcessId Process ID that was attached
	 */
	virtual void OnProcessAttach(uint32_t ProcessId) = 0;

	/**
	 * @brief Called when a process is detached in Cheat Engine
	 * @param ProcessId Process ID that was detached
	 */
	virtual void OnProcessDetach(uint32_t ProcessId) = 0;
};
