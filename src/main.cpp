#include "logger.h"
#include "FreeCamHandler.h"
#include "DrawMenu.h"
#include "Hooks.h"
#include "DebugMenu/DebugMenu.h"
#include "DebugUIMenu.h"
#include "MCM.h"
#include "Renderer/Renderer.h"
#include "Interface/UIHandler.h"
#include "Interface/InputHandler.h"
#include "TestHandler.h"
#include "DebugMenuAPI.h"
#include "ModAPI.h"
#include "SMFMenu.h"

//#define ENABLE_TESTING

#ifdef DEBUGMENU_NG
#	ifndef SKSEAPI
#		define SKSEAPI __cdecl  // what the SKSE headers define it as; alandtse's CommonLibSSE-NG leaves it out
#	endif
#endif

void MessageHandler(SKSE::MessagingInterface::Message* a_message) {
    switch (a_message->type) 
	{
		case SKSE::MessagingInterface::kPostLoad:
		{
			Hooks::Hook_CellLoad::install();
			Hooks::Hook_NavMeshLoad::install();
			break;
		}
		case SKSE::MessagingInterface::kPostPostLoad:
		{
			MCM::Register();
			MCM::InitNonMCMSettings();
			MCM::DebugMenuPresets::Init();

			DrawMenu::Register();
			DebugMenuUI::Register();
			DebugMenu::GetDebugMenuHandler()->Init();

			Hooks::Hook_SoundMarkersSetFormEditorID::install();
			//Hooks::Hook_LandLoad::install(); // landscape scroller
			




			break;
		}
        case SKSE::MessagingInterface::kDataLoaded: 
		{
		#ifdef ENABLE_TESTING
			RE::BSInputDeviceManager::GetSingleton()->AddEventSink(TestHandler::GetSingleton());
		#endif

			RE::BSInputDeviceManager::GetSingleton()->AddEventSink(FreeCamHandler::GetSingleton());
			RE::BSInputDeviceManager::GetSingleton()->AddEventSink(ScaleformUI::InputHandler::GetSingleton());
			RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESCellFullyLoadedEvent>(DebugMenu::GetNavmeshHandler().get());
			RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(DebugMenu::GetDebugMenuHandler().get());

			DebugMenu::GetMarkerHandler()->InitPostDataLoaded();
			FreeCamHandler::GetSingleton()->Init();

			ScaleformUI::UIHandler::GetSingleton()->Init();
			ScaleformUI::InputHandler::GetSingleton()->Init();

			// Updates
			SKSE::AllocTrampoline(14*3);
			Hooks::Hook_MainUpdate_sub_140437D40::Install();
			ScaleformUI::Hook_MainUpdate::Install();

			// Free cam
			Hooks::Hook_FreeCameraState_UpdatePosition_1408E0640::Install();
			Hooks::Hook_FreeCameraBegin::install();
			Hooks::Hook_FreeCameraEnd::install();

			SMFMenu::Register();

			MCM::settings::logUI = false;
			MCM::settings::uiScale = 1.0f;

			
			//bool* bShowMarkers = (bool*)RELOCATION_ID(508943, 381019).address();
			//*bShowMarkers = true;

			if (!MCM::settings::modActive)
			{
				logger::debug("Plugin currently disabled in MCM");
			}
			break;
        }
    }
}

static bool DebugMenuLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    SetupLog();
	
	logger::debug("plugin loaded");

	//// Use to attach debugger to skyrim very early.
	//// When attaching CE to skyrim, use the processes tab instead of the applications tab
	/*auto time = std::chrono::system_clock::now();
	int waitSeconds = 10;*/

	/*while (true)
	{
		auto newTime = std::chrono::system_clock::now();
		auto passedTime = std::chrono::duration_cast<std::chrono::seconds>(newTime-time).count();
		if (passedTime > waitSeconds)
		{
			break;
		}
	}*/

	MCM::DebugMenuMCM::ReadSettings(true);
	MCM::settings::useD3D = MCM::settings::useD3DMCMSetting;

	if (!MCM::settings::useD3D)
	{
		logger::debug("D3D11 is disabled in MCM");
	}


    const auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(MessageHandler);

	if (MCM::settings::useD3D)
	{
		logger::debug("D3D11 is enabled in MCM");

		if (!Renderer::AttachD3D())
		{
			logger::debug("AttachD3D failed");
			WarningPopup(L"DebugMenu: AttachD3D failed. D3D11 rendering will be disabled");
			MCM::settings::useD3D = false;
		}
	}
	
    return true;
}

#ifdef DEBUGMENU_NG
// ng build (alandtse CommonLibSSE-NG, Skyrim 1.6.1170 and 1.7.x): add_commonlibsse_plugin generates the SKSEPluginInfo block
SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	return DebugMenuLoad(a_skse);
}
#else
extern "C" __declspec( dllexport ) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	return DebugMenuLoad(a_skse);
}

extern "C" __declspec( dllexport ) constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::VERSION);
	v.PluginName(Version::NAME);
	v.AuthorName("Spodermon");
	v.UsesAddressLibrary();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });
	v.UsesNoStructs();

	return v;
}();

extern "C" __declspec( dllexport ) bool SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}
#endif

extern "C" __declspec(dllexport) void* SKSEAPI RequestPluginAPI(const DEBUGMENU_API::InterfaceVersion a_interfaceVersion)
{
	auto api = Messaging::DebugMenuInterface::GetSingleton();

	logger::info("DebguMenu::RequestPluginAPI called, InterfaceVersion {}", static_cast<uint8_t>(a_interfaceVersion));

	switch (a_interfaceVersion) {
		case DEBUGMENU_API::InterfaceVersion::V1:
			logger::info("DebugMenu::RequestPluginAPI returned the API singleton");
			return static_cast<void*>(api);
	}

	logger::info("DebugMenu::RequestPluginAPI requested the wrong interface version");
	return nullptr;
}
