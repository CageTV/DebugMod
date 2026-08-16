#pragma once

#include "DebugMenuAPI.h"

namespace Messaging
{
	using InterfaceVersion1 = ::DEBUGMENU_API::IVDebugMenu1;

	class DebugMenuInterface : public InterfaceVersion1
	{
		private:
			DebugMenuInterface() noexcept;
			virtual ~DebugMenuInterface() noexcept;

		public:
			static DebugMenuInterface* GetSingleton() noexcept
			{
				static DebugMenuInterface singleton;
				return std::addressof(singleton);
			}

			virtual void ToggleDebugMenuUI() noexcept override;
			virtual bool IsDebugMenuUIClosed() noexcept override;
			virtual bool IsDebugMenuUICurrentlyClosing() noexcept override;
			virtual void SetEnableDebugMenuUIHotkey(bool a_enabled) noexcept override;
	};
}