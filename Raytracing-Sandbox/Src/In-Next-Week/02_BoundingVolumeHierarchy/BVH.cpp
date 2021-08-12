#include "BVH.h"
#include <GLCore/Core/KeyCodes.h>
#include <GLCore/Core/MouseButtonCodes.h>
#include <GLCore/Core/Input.h>
#include <GLCore/Events/MouseEvent.h>

void BVH::OnDetach ()
{
	OnDetachBase ();
}
void BVH::OnAttach ()
{
	OnAttachBase ();
}
void BVH::OnUpdate (GLCore::Timestep ts)
{
	OnUpdateBase (ts);

	{ // normal drawing pass
		glClear (GL_COLOR_BUFFER_BIT);
		glUseProgram (m_SquareShaderProgID);
		glBindVertexArray (m_QuadVA);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputColorTextureID);
		glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}
}
void BVH::OnEvent (GLCore::Event &event)
{
	GLCore::EventDispatcher dispatcher (event);
	dispatcher.Dispatch<GLCore::LayerViewportResizeEvent> (
		[&](GLCore::LayerViewportResizeEvent &e)  -> bool {
			float AspectRatio = float (e.GetWidth ())/e.GetHeight ();

			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputColorTextureID);
			// (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, uint32_t(m_OutputResolutionHeight*AspectRatio), uint32_t(m_OutputResolutionHeight), 0, GL_RGBA, GL_FLOAT, nullptr);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputDepthTextureID);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, uint32_t(m_OutputResolutionHeight*AspectRatio), uint32_t(m_OutputResolutionHeight), 0, GL_RGBA, GL_FLOAT, nullptr);

			return false;
		});
	dispatcher.Dispatch<GLCore::KeyPressedEvent> (
		[&](GLCore::KeyPressedEvent &e)  -> bool {

			glm::vec3 front = m_Camera.Front ();
			glm::vec3 right = glm::cross (front, { 0,1,0 });

			switch (e.GetKeyCode())
			{
			case GLCore::Key::Up:
				m_Camera.pitch -= glm::radians (0.5f);
				break;
			case GLCore::Key::Down:
				m_Camera.pitch += glm::radians (0.5f);
				break;
			case GLCore::Key::Right:
				m_Camera.yaw += glm::radians (0.5f);
				break;
			case GLCore::Key::Left:
				m_Camera.yaw -= glm::radians (0.5f);
				break;
			case GLCore::Key::W:
				m_Camera.Position += (front*0.1f);
				break;
			case GLCore::Key::A:
				m_Camera.Position -= (right*0.1f);
				break;
			case GLCore::Key::S:
				m_Camera.Position -= (front*0.1f);
				break;
			case GLCore::Key::D:
				m_Camera.Position += (right*0.1f);
				break;
			}
			return true; 
		});
	dispatcher.Dispatch<GLCore::MouseMovedEvent> (
		[&](GLCore::MouseMovedEvent &e) -> bool {
			//if (GLCore::Input::IsMouseButtonPressed(GLCore::Mouse::Button1))
			//{
			//}
			LOG_TRACE ("mouse {0}, {1}", e.GetX (), e.GetY ());
			return false;
		});
}
void BVH::OnImGuiRender ()
{
	ImGui::Begin (GLCore::ImGuiLayer::UniqueName ("Settings"));
	if (ImGui::BeginTabBar ("#.Options.#")) {
		if (ImGui::BeginTabItem ("RT-Options")) {

			OnImGuiRenderBase ();

			ImGui::EndTabItem ();
		}
		if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Compute Shader Source"))) {

			OnImGuiComputeShaderSource ();

			ImGui::EndTabItem ();
		}
		if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Square Shader Source"))) {

			OnImGuiSqureShaderSource ();

			ImGui::EndTabItem ();
		}
		ImGui::EndTabBar ();
	}
	ImGui::End ();
}
void BVH::OnComputeShaderReload ()
{
	OnComputeShaderReloadBase ();
}
void BVH::OnSquareShaderReload ()
{}

bool BVH::FillBuffer ()
{
	bool reupload_buffer = false;

	for (uint32_t i = 0; i < m_GeometryData.size (); i++)
		if (m_GeometryData[i].updated)
			m_GeometryData[i].FillBuffer (&m_GeometryBuffer[i]), m_GeometryData[i].updated = false, reupload_buffer = true;

	return reupload_buffer;
}