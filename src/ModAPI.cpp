#include "ModAPI.h"
#include "Interface/UIHandler.h"
#include "MCM.h"

namespace Messaging
{

	DebugMenuInterface::DebugMenuInterface() noexcept {}
	DebugMenuInterface::~DebugMenuInterface() noexcept {}

	void DebugMenuInterface::ToggleDebugMenuUI() noexcept
	{
		if (auto& debugMenuUI = ScaleformUI::UIHandler::GetSingleton()->debugMenuUI)
			debugMenuUI->ToggleMenu();
	}

	bool DebugMenuInterface::IsDebugMenuUIClosed() noexcept
	{
		if (auto& debugMenuUI = ScaleformUI::UIHandler::GetSingleton()->debugMenuUI)
		{
			return debugMenuUI->IsClosed();
		}
		return true;
	}

	bool DebugMenuInterface::IsDebugMenuUICurrentlyClosing() noexcept
	{
		if (auto& debugMenuUI = ScaleformUI::UIHandler::GetSingleton()->debugMenuUI)
		{
			return debugMenuUI->IsCurrentlyClosing();
		}
		return true;
	}

	void DebugMenuInterface::SetEnableDebugMenuUIHotkey(bool a_enabled) noexcept
	{
		MCM::settings::enableMenuOpenOnHotkey = a_enabled;

		if (auto& debugMenuUI = ScaleformUI::UIHandler::GetSingleton()->debugMenuUI)
		{
			if (a_enabled)
				debugMenuUI->SetMenuOpenKeyCode(MCM::settings::openMenuHotkey);
			else 
				debugMenuUI->SetMenuOpenKeyCode(0);
		}
	}
}