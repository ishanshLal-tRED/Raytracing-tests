#pragma once

namespace GLCore
{
	class TestBase;

	class TestsLayerManager
	{
	public:
		TestsLayerManager () = default;
		~TestsLayerManager ();

		void PushTest (TestBase* test);

		void UpdateActiveLayers (Timestep deltatime);
		void ImGuiRender ();
		void OnImGuiRenderAll ();
		void ProcessEvent (Event &event);

		void ActivateTest (uint16_t posn);
		void DeActivateTest (uint16_t posn);
	private:
		void ShowTestMenu ();
		bool m_ShowTestMenu = false;
	private:
		uint32_t m_DockspaceID;

		TestBase* m_ActiveTests[2] = { nullptr, nullptr };
		std::vector <TestBase*>   m_AllTests;
	};
}