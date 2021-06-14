#include "ExampleLayer.h"
#include "Compute-Shader/02_Evolving_Pics/APT.h"

using namespace GLCore;
using namespace GLCore::Utils;

ExampleLayer::ExampleLayer(std::string name)
	: TestBase(name, "Just a Square"), m_CameraController(16.0f / 9.0f)
{

}

void ExampleLayer::OnAttach()
{
	EnableGLDebugging();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_Shader = Shader::FromGLSLTextFiles(
		"assets/shaders/test.vert.glsl",
		"assets/shaders/test.frag.glsl"
	);

	glGenVertexArrays(1, &m_QuadVA);
	glBindVertexArray(m_QuadVA);

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		 0.5f,  0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f
	};

	glGenBuffers(1, &m_QuadVB);
	glBindBuffer(GL_ARRAY_BUFFER, m_QuadVB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

	uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
	glGenBuffers(1, &m_QuadIB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_QuadIB);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void ExampleLayer::OnDetach()
{
	glDeleteVertexArrays(1, &m_QuadVA);
	glDeleteBuffers(1, &m_QuadVB);
	glDeleteBuffers(1, &m_QuadIB);
}

void ExampleLayer::OnEvent(Event& event)
{
	m_CameraController.OnEvent(event);

	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<MouseButtonPressedEvent>(
		[&](MouseButtonPressedEvent& e)
		{
			m_SquareColor = m_SquareAlternateColor;
			return false;
		});
	dispatcher.Dispatch<MouseButtonReleasedEvent>(
		[&](MouseButtonReleasedEvent& e)
		{
			m_SquareColor = m_SquareBaseColor;
			return false;
		});
	dispatcher.Dispatch<MouseMovedEvent> (
		[&](MouseMovedEvent &e) {
			LOG_TRACE ("Mouse posn [{0}, {1}]", e.GetX (), e.GetY ());
			return true;
		});
	dispatcher.Dispatch<LayerCloseEvent> (
		[&](LayerCloseEvent &e) {
			LOG_TRACE ("Layer Closed");
			return true;
		});
	dispatcher.Dispatch<LayerViewportFocusEvent> (
		[&](LayerViewportFocusEvent &e) {
			LOG_TRACE ("ViewPort Focused");
			return true;
		});
	dispatcher.Dispatch<LayerViewportLostFocusEvent> (
		[&](LayerViewportLostFocusEvent &e) {
			LOG_TRACE ("ViewPort lost Focused");
			return true;
		});
}

void ExampleLayer::OnUpdate(Timestep ts)
{
	m_CameraController.OnUpdate(ts);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(m_Shader->GetRendererID());

	int location = glGetUniformLocation(m_Shader->GetRendererID(), "u_ViewProjection");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m_CameraController.GetCamera().GetViewProjectionMatrix()));

	location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glUniform4fv(location, 1, glm::value_ptr(m_SquareColor));

	glBindVertexArray(m_QuadVA);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
std::array<int, 10> just_an_arr = my_std::make_array<10> (23);
void ExampleLayer::OnImGuiRender()
{
	ImGui::Begin(ImGuiLayer::UniqueName("Controls"));
	if (ImGui::ColorEdit4("Square Base Color", glm::value_ptr(m_SquareBaseColor)))
		m_SquareColor = m_SquareBaseColor;
	ImGui::ColorEdit4("Square Alternate Color", glm::value_ptr(m_SquareAlternateColor));
	ImGui::Text ("just an array:");
	for (uint32_t i = 0; i < just_an_arr.size (); i++)
		ImGui::Text ("  %d", just_an_arr[i]);

	ImGui::End();
}

void ExampleLayer::ImGuiMenuOptions ()
{
	const char* menuName = "Testing_Menu";
	const char *newMenuName = ImGuiLayer::UniqueName(menuName);
	if (ImGui::BeginMenu (newMenuName)) {

		if (ImGui::MenuItem ("Close This Test")) {
			int nameLen = strlen(menuName);
			int newNameLen = strlen(newMenuName);
			if(nameLen < newNameLen){
				Application::Get ().DeActivateLayer(atoi(&newMenuName[nameLen + 1]) - 1);
			}else{
				Application::Get ().DeActivateLayer(0);
			}
		}
		ImGui::Separator ();

		ImGui::EndMenu ();
	}
}