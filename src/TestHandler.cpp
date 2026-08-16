#include "TestHandler.h"
#include "MCM.h"

RE::BSEventNotifyControl TestHandler::ProcessEvent(RE::InputEvent* const* eventPtr, RE::BSTEventSource<RE::InputEvent*>*)
{
	if (!eventPtr) return RE::BSEventNotifyControl::kContinue;

	for (auto* event = *eventPtr; event; event = event->next)
	{
		if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) continue;

		uint32_t offset = 0;
		if (event->GetDevice() == RE::INPUT_DEVICE::kMouse) offset = 256;

		auto* buttonEvent = event->AsButtonEvent();
		auto dxScanCode = buttonEvent->GetIDCode();

		uint32_t keyCode = dxScanCode + offset;

		if (keyCode == testKey && buttonEvent->IsDown())
		{
			DoTest();
		}
	}
	return RE::BSEventNotifyControl::kContinue;
}

void TestHandler::DoTest()
{
	return;
	const auto audioManager = RE::BSAudioManager::GetSingleton();
	const auto& activeSounds = audioManager->activeSounds;

	for (const auto& [id, sound] : activeSounds)
	{
		//const RE::
		//const auto& handle = audioManager->GetSoundHandleByFile()
	}
}