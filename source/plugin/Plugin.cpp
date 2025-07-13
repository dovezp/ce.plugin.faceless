#include "Plugin.h"
#include "PluginDetails.h"
#include "Logger.h"

#include <sstream>

#define CLASS_NAME_TAG __FILE__

Plugin::Plugin()
	: AntiScreenCaptureSystem(nullptr)
	, CurrentState(EPluginState::Uninitialized)
	, bIsProtectionActive(false)
	, bIsAutoProtectionEnabled(true)
	, bIsInitialized(false)
{
}

Plugin::~Plugin()
{
	Shutdown();
}

bool Plugin::Initialize()
{
	if (bIsInitialized.load())
	{
		return true;
	}

	if (!SetPluginState(EPluginState::Initializing))
	{
		return false;
	}

	if (!Settings::GetInstance().Initialize())
	{
		Log("[%s] Failed to initialize settings", CLASS_NAME_TAG);
		SetPluginState(EPluginState::Error);
		return false;
	}

	if (!InitializeSubsystems())
	{
		Log("[%s] Failed to initialize subsystems", CLASS_NAME_TAG);
		SetPluginState(EPluginState::Error);
		return false;
	}

	bIsInitialized.store(true);
	SetPluginState(EPluginState::Inactive);

	const auto& AntiCaptureConfig = Settings::GetInstance().GetAntiScreenCaptureConfig();
	if (AntiCaptureConfig.bEnabled)
	{
		StartProtection();
	}

	return true;
}

void Plugin::Shutdown()
{
	if (!bIsInitialized.load())
	{
		return;
	}

	SetPluginState(EPluginState::Shutting_Down);
	StopProtection();
	ShutdownSubsystems();

	bIsInitialized.store(false);
	SetPluginState(EPluginState::Uninitialized);
}

bool Plugin::InitializeSubsystems()
{
	AntiScreenCaptureSystem = std::make_unique<AntiScreenCapture>();
	if (!AntiScreenCaptureSystem->Initialize())
	{
		return false;
	}
	return true;
}

void Plugin::ShutdownSubsystems()
{
	if (AntiScreenCaptureSystem)
	{
		AntiScreenCaptureSystem->Shutdown();
		AntiScreenCaptureSystem.reset();
	}
}

bool Plugin::StartProtection()
{
	if (!bIsInitialized.load())
	{
		return false;
	}

	if (bIsProtectionActive.load())
	{
		return true;
	}

	const auto& Config = Settings::GetInstance().GetAntiScreenCaptureConfig();
	if (!Config.bEnabled)
	{
		Log("[%s] Protection feature is disabled", CLASS_NAME_TAG);
		return false;
	}

	uint32_t ProtectionMethods = static_cast<uint32_t>(AntiScreenCapture::EProtectionMethod::DisplayAffinity);
	auto Result = AntiScreenCaptureSystem->ApplyProtectionToCurrentProcess(ProtectionMethods);

	if (Result == AntiScreenCapture::EProtectionResult::Success ||
		Result == AntiScreenCapture::EProtectionResult::PartialSuccess ||
		Result == AntiScreenCapture::EProtectionResult::AlreadyProtected)
	{
		bIsProtectionActive.store(true);
		SetPluginState(EPluginState::Active);

		if (bIsAutoProtectionEnabled.load())
		{
			uint32_t CurrentProcessId = GetCurrentProcessId();
			AntiScreenCaptureSystem->StartWindowEventHook(CurrentProcessId);
		}

		Log("[%s] Protection started", CLASS_NAME_TAG);
		return true;
	}

	SetPluginState(EPluginState::Error);
	return false;
}

bool Plugin::StopProtection()
{
	if (!bIsProtectionActive.load())
	{
		return true;
	}

	if (AntiScreenCaptureSystem && AntiScreenCaptureSystem->IsWindowEventHookActive())
	{
		AntiScreenCaptureSystem->StopWindowEventHook();
	}

	if (AntiScreenCaptureSystem)
	{
		auto Result = AntiScreenCaptureSystem->RemoveProtectionFromProcess(GetCurrentProcessId());
		Log("[%s] Protection Stopped with EProtectionResult=%d", CLASS_NAME_TAG, Result);
	}

	bIsProtectionActive.store(false);
	SetPluginState(EPluginState::Inactive);
	return true;
}

bool Plugin::IsProtectionActive() const
{
	return bIsProtectionActive.load();
}

bool Plugin::ProtectCurrentProcess()
{
	if (!bIsInitialized.load() || !AntiScreenCaptureSystem)
	{
		return false;
	}

	const auto& Config = Settings::GetInstance().GetAntiScreenCaptureConfig();
	uint32_t ProtectionMethods = static_cast<uint32_t>(AntiScreenCapture::EProtectionMethod::DisplayAffinity);

	auto Result = AntiScreenCaptureSystem->ApplyProtectionToCurrentProcess(ProtectionMethods);

	return (Result == AntiScreenCapture::EProtectionResult::Success ||
		Result == AntiScreenCapture::EProtectionResult::PartialSuccess ||
		Result == AntiScreenCapture::EProtectionResult::AlreadyProtected);
}

bool Plugin::UnprotectCurrentProcess()
{
	if (!bIsInitialized.load() || !AntiScreenCaptureSystem)
	{
		return false;
	}

	auto Result = AntiScreenCaptureSystem->RemoveProtectionFromProcess(GetCurrentProcessId());

	return (Result == AntiScreenCapture::EProtectionResult::Success ||
		Result == AntiScreenCapture::EProtectionResult::PartialSuccess);
}

void Plugin::OnConfigurationChanged()
{
	const auto& AntiCaptureConfig = Settings::GetInstance().GetAntiScreenCaptureConfig();

	if (AntiCaptureConfig.bEnabled && !bIsProtectionActive.load())
	{
		StartProtection();
	}
	else if (!AntiCaptureConfig.bEnabled && bIsProtectionActive.load())
	{
		StopProtection();
	}
}

Plugin::EPluginState Plugin::GetPluginState() const
{
	return CurrentState.load();
}

void Plugin::SetAutoProtectionEnabled(bool bEnabled)
{
	bIsAutoProtectionEnabled.store(bEnabled);
}

bool Plugin::IsAutoProtectionEnabled() const
{
	return bIsAutoProtectionEnabled.load();
}

bool Plugin::SetPluginState(EPluginState NewState)
{
	if (!IsValidStateTransition(NewState))
	{
		return false;
	}

	EPluginState OldState = CurrentState.exchange(NewState);

	return true;
}

bool Plugin::IsValidStateTransition(EPluginState NewState) const
{
	EPluginState CurrentStateValue = CurrentState.load();

	if (CurrentStateValue == NewState)
	{
		return true;
	}

	if (NewState == EPluginState::Error)
	{
		return true;
	}

	if (NewState == EPluginState::Shutting_Down && CurrentStateValue != EPluginState::Uninitialized)
	{
		return true;
	}

	switch (CurrentStateValue)
	{
	case EPluginState::Uninitialized:
		return NewState == EPluginState::Initializing;

	case EPluginState::Initializing:
		return NewState == EPluginState::Inactive || NewState == EPluginState::Active;

	case EPluginState::Inactive:
		return NewState == EPluginState::Active;

	case EPluginState::Active:
		return NewState == EPluginState::Inactive;

	case EPluginState::Error:
		return NewState == EPluginState::Inactive || NewState == EPluginState::Uninitialized;

	case EPluginState::Shutting_Down:
		return NewState == EPluginState::Uninitialized;

	default:
		return false;
	}
}

std::string Plugin::GetVersion()
{
	return PLUGIN_VERSION_STRING;
}

std::string Plugin::GetName()
{
	return PLUGIN_NAME;
}

std::string Plugin::GetDescription()
{
	return PLUGIN_DESCRIPTION;
}

bool Plugin::OnPluginLoad()
{
	return Initialize();
}

void Plugin::OnPluginUnload()
{
	Shutdown();
}

void Plugin::OnProcessAttach(uint32_t ProcessId)
{
	if (bIsAutoProtectionEnabled.load() && AntiScreenCaptureSystem)
	{
		const auto& Config = Settings::GetInstance().GetAntiScreenCaptureConfig();
		uint32_t ProtectionMethods = static_cast<uint32_t>(AntiScreenCapture::EProtectionMethod::DisplayAffinity);
		AntiScreenCaptureSystem->ApplyProtectionToProcess(ProcessId, ProtectionMethods);
	}
}

void Plugin::OnProcessDetach(uint32_t ProcessId)
{
	if (AntiScreenCaptureSystem)
	{
		AntiScreenCaptureSystem->RemoveProtectionFromProcess(ProcessId);
	}
}