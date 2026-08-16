#pragma once

class TestHandler : public RE::BSTEventSink<RE::InputEvent*>
{
	public:
		static TestHandler* GetSingleton()
		{
			static TestHandler singleton;
			return std::addressof(singleton);
		}

		const uint32_t testKey = 258;

		RE::BSEventNotifyControl	ProcessEvent(RE::InputEvent* const* eventPtr, RE::BSTEventSource<RE::InputEvent*>*);
		void						DoTest();
};