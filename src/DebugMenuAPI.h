#pragma once

/*
* For modders: Copy this file into your own project if you wish to use this API
*/

namespace DEBUGMENU_API
{
	constexpr const auto DebugMenuPluginName = "DebugMenu.dll";

	enum class InterfaceVersion : uint8_t
	{
		V1 = 1
	};

	class IVDebugMenu1
	{
		public:

			// Opens UI if it is closed or is in closing animation
			// Run as UI Task
			virtual void ToggleDebugMenuUI() noexcept = 0;

			// Is the UI closed?
			virtual bool IsDebugMenuUIClosed() noexcept = 0;
			
			// Is the UI in a closing animation?
			virtual bool IsDebugMenuUICurrentlyClosing() noexcept = 0;

			// Enable/disable UI open hotkey
			virtual void SetEnableDebugMenuUIHotkey(bool a_enabled) noexcept = 0;
	};

	typedef void* (*_RequestPluginAPI)(const InterfaceVersion interfaceVersion);

	/// <summary>
	/// Request the Debug Menu API interface.
	/// Recommended: Send your request during or after SKSEMessagingInterface::kMessage_PostLoad to make sure the dll has already been loaded
	/// </summary>
	/// <param name="a_interfaceVersion">The interface version to request</param>
	/// <returns>The pointer to the API singleton, or nullptr if request failed</returns>
	[[nodiscard]] inline void* RequestPluginAPI(const InterfaceVersion a_interfaceVersion = InterfaceVersion::V1)
	{
		auto pluginHandle = GetModuleHandleA("DebugMenu.dll");
		_RequestPluginAPI requestAPIFunction = (_RequestPluginAPI)GetProcAddress(pluginHandle, "RequestPluginAPI");
		if (requestAPIFunction) {
			return requestAPIFunction(a_interfaceVersion);
		}
		return nullptr;
	}
}