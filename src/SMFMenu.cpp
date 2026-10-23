#include "SMFMenu.h"
#include "MCM.h"

#include <SimpleIni.h>
#include <atomic>
#include <filesystem>
#include <format>

#pragma warning(push)
#pragma warning(disable : 4099 5054)
#include "SKSEMenuFramework.h"
#pragma warning(pop)

namespace SMFMenu
{
	namespace
	{
		constexpr auto defaultSettingsPath = "Data/MCM/Config/DebugMenu/settings.ini";
		constexpr auto userSettingsPath = "Data/MCM/Settings/DebugMenu.ini";

		enum class Kind { Header, Bool, UInt, Float, Key, Color, Enum };

		struct Entry
		{
			int			page;
			Kind		kind;
			const char* section;
			const char* key;
			void*		member;	// bool*, uint32_t* or float* into MCM::settings
			const char* label;
			const char* help;
			float		lo;
			float		hi;
			const char* format;	// slider format, or the zero separated option list of an enum
		};

		// SMFSettings.inc is generated from the MCM config by tools/gen_smf_settings.py
#define SMF_HEADER(p, t)							{ p, Kind::Header, nullptr, nullptr, nullptr, t, nullptr, 0, 0, nullptr },
#define SMF_BOOL(p, s, k, m, l, h)					{ p, Kind::Bool,  s, k, m, l, h, 0, 0, nullptr },
#define SMF_KEY(p, s, k, m, l, h)					{ p, Kind::Key,   s, k, m, l, h, 0, 0, nullptr },
#define SMF_COLOR(p, s, k, m, l, h)					{ p, Kind::Color, s, k, m, l, h, 0, 0, nullptr },
#define SMF_ENUM(p, s, k, m, l, h, opts)			{ p, Kind::Enum,  s, k, m, l, h, 0, 0, opts },
#define SMF_UINT(p, s, k, m, l, h, lo, hi, f)		{ p, Kind::UInt,  s, k, m, l, h, (float)(lo), (float)(hi), f },
#define SMF_FLOAT(p, s, k, m, l, h, lo, hi, f)		{ p, Kind::Float, s, k, m, l, h, lo, hi, f },
		const std::vector<Entry> entries = {
#include "SMFSettings.inc"
		};
#undef SMF_HEADER
#undef SMF_BOOL
#undef SMF_KEY
#undef SMF_COLOR
#undef SMF_ENUM
#undef SMF_UINT
#undef SMF_FLOAT

		const char* const pageNames[] = { SMF_PAGE_NAMES };
		constexpr int pageCount = sizeof(pageNames) / sizeof(pageNames[0]);

		// What the widgets edit. Nothing here touches the live settings until Save: several settings
		// (the "on menu open" ones) are only read when the game starts, exactly like in the MCM, and the
		// live variables of those are the current display state, which an edit must not change.
		std::vector<double>	staged;
		bool				stagedLoaded = false;
		bool				dirty = false;

		double ReadLive(const Entry& e)
		{
			switch (e.kind)
			{
				case Kind::Bool:	return *static_cast<bool*>(e.member) ? 1.0 : 0.0;
				case Kind::Float:	return *static_cast<float*>(e.member);
				case Kind::Header:	return 0.0;
				default:			return *static_cast<uint32_t*>(e.member);
			}
		}

		void ReadIni(CSimpleIniA& a_ini, const Entry& e, double& a_value)
		{
			if (!a_ini.GetValue(e.section, e.key)) return;
			switch (e.kind)
			{
				case Kind::Bool:	a_value = a_ini.GetBoolValue(e.section, e.key) ? 1.0 : 0.0; break;
				case Kind::Float:	a_value = a_ini.GetDoubleValue(e.section, e.key); break;
				case Kind::Header:	break;
				default:			a_value = static_cast<double>(static_cast<uint32_t>(a_ini.GetLongValue(e.section, e.key))); break;
			}
		}

		// a_defaultsOnly: what the mod ships with, ignoring the user's ini
		void LoadStaged(bool a_defaultsOnly)
		{
			CSimpleIniA defaults;
			defaults.SetUnicode();
			defaults.LoadFile(defaultSettingsPath);
			CSimpleIniA user;
			user.SetUnicode();
			if (!a_defaultsOnly) user.LoadFile(userSettingsPath);

			staged.assign(entries.size(), 0.0);
			for (size_t i = 0; i < entries.size(); i++)
			{
				staged[i] = ReadLive(entries[i]);
				ReadIni(defaults, entries[i], staged[i]);
				if (!a_defaultsOnly) ReadIni(user, entries[i], staged[i]);
			}
			stagedLoaded = true;
		}

		void SaveStaged()
		{
			CSimpleIniA user;
			user.SetUnicode();
			user.LoadFile(userSettingsPath);

			for (size_t i = 0; i < entries.size(); i++)
			{
				const Entry& e = entries[i];
				switch (e.kind)
				{
					case Kind::Header:	break;
					case Kind::Float:	user.SetDoubleValue(e.section, e.key, staged[i]); break;
					default:			user.SetLongValue(e.section, e.key, static_cast<long>(staged[i])); break; // bools as 0/1, like the MCM does
				}
			}

			std::error_code ec;
			std::filesystem::create_directories(std::filesystem::path(userSettingsPath).parent_path(), ec);
			if (user.SaveFile(userSettingsPath) < 0)
			{
				logger::error("Failed to save {}", userSettingsPath);
				return;
			}
			dirty = false;

			// re-read and apply on the main thread, same as closing the MCM
			SKSE::GetTaskInterface()->AddTask([]() {
				MCM::DebugMenuMCM::OnConfigClose(nullptr);
			});
		}

		// DirectInput scan code <-> ImGui key, for the hotkey settings (the game wants scan codes)
		struct KeyPair { uint32_t scan; ImGuiMCP::ImGuiKey key; };
		using namespace ImGuiMCP;
		const KeyPair keyTable[] = {
			{ 1, ImGuiKey_Escape }, { 2, ImGuiKey_1 }, { 3, ImGuiKey_2 }, { 4, ImGuiKey_3 }, { 5, ImGuiKey_4 },
			{ 6, ImGuiKey_5 }, { 7, ImGuiKey_6 }, { 8, ImGuiKey_7 }, { 9, ImGuiKey_8 }, { 10, ImGuiKey_9 },
			{ 11, ImGuiKey_0 }, { 12, ImGuiKey_Minus }, { 13, ImGuiKey_Equal }, { 14, ImGuiKey_Backspace },
			{ 15, ImGuiKey_Tab }, { 16, ImGuiKey_Q }, { 17, ImGuiKey_W }, { 18, ImGuiKey_E }, { 19, ImGuiKey_R },
			{ 20, ImGuiKey_T }, { 21, ImGuiKey_Y }, { 22, ImGuiKey_U }, { 23, ImGuiKey_I }, { 24, ImGuiKey_O },
			{ 25, ImGuiKey_P }, { 26, ImGuiKey_LeftBracket }, { 27, ImGuiKey_RightBracket }, { 28, ImGuiKey_Enter },
			{ 29, ImGuiKey_LeftCtrl }, { 30, ImGuiKey_A }, { 31, ImGuiKey_S }, { 32, ImGuiKey_D }, { 33, ImGuiKey_F },
			{ 34, ImGuiKey_G }, { 35, ImGuiKey_H }, { 36, ImGuiKey_J }, { 37, ImGuiKey_K }, { 38, ImGuiKey_L },
			{ 39, ImGuiKey_Semicolon }, { 40, ImGuiKey_Apostrophe }, { 41, ImGuiKey_GraveAccent },
			{ 42, ImGuiKey_LeftShift }, { 43, ImGuiKey_Backslash }, { 44, ImGuiKey_Z }, { 45, ImGuiKey_X },
			{ 46, ImGuiKey_C }, { 47, ImGuiKey_V }, { 48, ImGuiKey_B }, { 49, ImGuiKey_N }, { 50, ImGuiKey_M },
			{ 51, ImGuiKey_Comma }, { 52, ImGuiKey_Period }, { 53, ImGuiKey_Slash }, { 54, ImGuiKey_RightShift },
			{ 55, ImGuiKey_KeypadMultiply }, { 56, ImGuiKey_LeftAlt }, { 57, ImGuiKey_Space }, { 58, ImGuiKey_CapsLock },
			{ 59, ImGuiKey_F1 }, { 60, ImGuiKey_F2 }, { 61, ImGuiKey_F3 }, { 62, ImGuiKey_F4 }, { 63, ImGuiKey_F5 },
			{ 64, ImGuiKey_F6 }, { 65, ImGuiKey_F7 }, { 66, ImGuiKey_F8 }, { 67, ImGuiKey_F9 }, { 68, ImGuiKey_F10 },
			{ 69, ImGuiKey_NumLock }, { 70, ImGuiKey_ScrollLock }, { 71, ImGuiKey_Keypad7 }, { 72, ImGuiKey_Keypad8 },
			{ 73, ImGuiKey_Keypad9 }, { 74, ImGuiKey_KeypadSubtract }, { 75, ImGuiKey_Keypad4 }, { 76, ImGuiKey_Keypad5 },
			{ 77, ImGuiKey_Keypad6 }, { 78, ImGuiKey_KeypadAdd }, { 79, ImGuiKey_Keypad1 }, { 80, ImGuiKey_Keypad2 },
			{ 81, ImGuiKey_Keypad3 }, { 82, ImGuiKey_Keypad0 }, { 83, ImGuiKey_KeypadDecimal }, { 87, ImGuiKey_F11 },
			{ 88, ImGuiKey_F12 }, { 156, ImGuiKey_KeypadEnter }, { 157, ImGuiKey_RightCtrl }, { 181, ImGuiKey_KeypadDivide },
			{ 183, ImGuiKey_PrintScreen }, { 184, ImGuiKey_RightAlt }, { 197, ImGuiKey_Pause }, { 199, ImGuiKey_Home },
			{ 200, ImGuiKey_UpArrow }, { 201, ImGuiKey_PageUp }, { 203, ImGuiKey_LeftArrow }, { 205, ImGuiKey_RightArrow },
			{ 207, ImGuiKey_End }, { 208, ImGuiKey_DownArrow }, { 209, ImGuiKey_PageDown }, { 210, ImGuiKey_Insert },
			{ 211, ImGuiKey_Delete },
		};

		ImGuiKey ScanToKey(uint32_t a_scan)
		{
			for (const auto& p : keyTable) if (p.scan == a_scan) return p.key;
			return ImGuiKey_None;
		}

		uint32_t KeyToScan(ImGuiKey a_key)
		{
			for (const auto& p : keyTable) if (p.key == a_key) return p.scan;
			return 0;
		}

		void Tip(const char* a_text)
		{
			if (a_text && a_text[0] && IsItemHovered())
			{
				SetTooltip("%s", a_text);
			}
		}

		void __stdcall OnMenuEvent(SKSEMenuFramework::Model::EventType a_type)
		{
			if (a_type == SKSEMenuFramework::Model::kCloseMenu && dirty)
			{
				SaveStaged();
			}
		}

		void RenderPage(int a_page)
		{
			if (!stagedLoaded) LoadStaged(false);

			if (a_page == 2)
			{
				TextDisabled("Settings on this page are read when the game starts. A change applies after a restart, as in the MCM.");
			}

			bool changed = false;
			for (size_t i = 0; i < entries.size(); i++)
			{
				const Entry& e = entries[i];
				if (e.page != a_page) continue;

				if (e.kind == Kind::Header)
				{
					SeparatorText(e.label);
					continue;
				}

				PushID(static_cast<int>(i));
				double& v = staged[i];
				bool c = false;

				switch (e.kind)
				{
					case Kind::Bool:
					{
						bool b = v != 0.0;
						if (Checkbox(e.label, &b)) { v = b ? 1.0 : 0.0; c = true; }
						break;
					}
					case Kind::UInt:
					{
						int n = static_cast<int>(v);
						if (SliderInt(e.label, &n, static_cast<int>(e.lo), static_cast<int>(e.hi), e.format)) { v = n; c = true; }
						break;
					}
					case Kind::Float:
					{
						float f = static_cast<float>(v);
						if (SliderFloat(e.label, &f, e.lo, e.hi, e.format)) { v = f; c = true; }
						break;
					}
					case Kind::Enum:
					{
						int n = static_cast<int>(v);
						if (Combo(e.label, &n, e.format)) { v = n; c = true; }
						break;
					}
					case Kind::Color:
					{
						const uint32_t rgb = static_cast<uint32_t>(v);
						float col[3] = { ((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f, (rgb & 0xFF) / 255.0f };
						if (ColorEdit3(e.label, col))
						{
							const auto to8 = [](float x) { return static_cast<uint32_t>(std::clamp(x, 0.0f, 1.0f) * 255.0f + 0.5f); };
							v = static_cast<double>((to8(col[0]) << 16) | (to8(col[1]) << 8) | to8(col[2]));
							c = true;
						}
						break;
					}
					case Kind::Key:
					{
						const uint32_t scan = static_cast<uint32_t>(v);
						ImGuiKey key = ScanToKey(scan);
						if (ImGuiMCPComponents::KeyBindingPicker(e.label, &key))
						{
							v = KeyToScan(key);
							c = true;
						}
						if (key == ImGuiKey_None && scan != 0)
						{
							SameLine();
							TextDisabled("(scan code %u)", scan);
						}
						break;
					}
					default: break;
				}
				Tip(e.help);
				PopID();
				changed |= c;
			}

			dirty |= changed;

			Spacing();
			Separator();
			if (Button("Save and apply"))
			{
				SaveStaged();
			}
			SameLine();
			if (Button("Reload from disk"))
			{
				LoadStaged(false);
				dirty = false;
			}
			SameLine();
			if (Button("Reset all to mod defaults"))
			{
				LoadStaged(true);
				dirty = true;
			}
			TextDisabled(dirty ? "Unsaved changes. They are saved when this menu closes." : "Saved to Data/MCM/Settings/DebugMenu.ini, the same file the MCM uses.");
		}

		template <int N>
		void __stdcall RenderPageN()
		{
			RenderPage(N);
		}

		using RenderFn = void(__stdcall*)();
		const RenderFn renderers[] = { RenderPageN<0>, RenderPageN<1>, RenderPageN<2>, RenderPageN<3>, RenderPageN<4> };
		static_assert(sizeof(renderers) / sizeof(renderers[0]) == pageCount, "one render function per MCM page");
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled())
		{
			logger::debug("SKSE Menu Framework is not installed, skipping its settings pages");
			return;
		}

		SKSEMenuFramework::SetSection("Debug Menu");
		for (int i = 0; i < pageCount; i++)
		{
			SKSEMenuFramework::AddSectionItem(pageNames[i], renderers[i]);
		}
		SKSEMenuFramework::AddEvent(OnMenuEvent, 0.0f); // never freed on purpose, like the framework's own examples

		logger::info("Registered {} pages with the SKSE Menu Framework", pageCount);
	}
}
