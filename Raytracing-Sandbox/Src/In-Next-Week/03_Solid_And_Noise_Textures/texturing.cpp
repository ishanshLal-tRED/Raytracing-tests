#include "texturing.h"
#include <GLCore/Core/KeyCodes.h>
#include <filesystem>

namespace In_Next_Week
{
	std::vector<GLuint> GeometryData_03::AllTextures = std::vector<GLuint> ();
	std::string GeometryData_03::AllTexturesStr = "None\0";
	uint32_t num_of_textures = 0;

	void Texturing::OnUpdate (GLCore::Timestep ts)
	{
		OnUpdateBase (ts, GeometryData_03::BindTextureOption);
		{ // normal drawing pass
			glClear (GL_COLOR_BUFFER_BIT);
			glUseProgram (m_SquareShaderProgID);
			glBindVertexArray (m_QuadVA);

			//glActiveTexture (GL_TEXTURE0);
			//glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputDepthTextureID);
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputColorTextureID);

			glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		}
	}
	void Texturing::OnAttach ()
	{
		m_NumberOfGeometriesToRender = 2;

		OnAttachBase ();
		static bool first = true;
		if (first) {
			first = false;
			GeometryData_03::AddTextureOption ("assets/dice.png");
		}

		LOG_TRACE ("GeomBuffSize: {0}, SceneNodeSize: {1}", sizeof (GeometryBuff_03), sizeof (LBVH::BVHNodeBuff));
		// Setup
		m_Camera.Position = glm::vec3 (-3, 6.5f, -3);
		m_Camera.pitch = -30;
		m_Camera.yaw = 45;
	}
	void Texturing::OnDetach ()
	{
		OnDetachBase ();
	}
	void Texturing::OnEvent (GLCore::Event &event)
	{
		GLCore::EventDispatcher dispatcher (event);
		dispatcher.Dispatch<GLCore::LayerViewportResizeEvent> (
			[&](GLCore::LayerViewportResizeEvent &e) -> bool {
				m_AspectRatio = float (e.GetWidth ())/float (e.GetHeight ());

				// Backup GL state
				GLenum last_active_texture; glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint *)&last_active_texture);
				GLuint last_texture; glGetIntegerv (GL_TEXTURE_BINDING_2D, (GLint *)&last_texture);

				// glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
				glActiveTexture (GL_TEXTURE0);
				glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputColorTextureID);
				glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, uint32_t (m_RTOutputResolution*m_AspectRatio), uint32_t (m_RTOutputResolution), 0, GL_RGBA, GL_FLOAT, nullptr);
				glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputDepthTextureID);
				glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, uint32_t (m_RTOutputResolution*m_AspectRatio), uint32_t (m_RTOutputResolution), 0, GL_RGBA, GL_FLOAT, nullptr);

				glActiveTexture (last_active_texture);
				glBindTexture (GL_TEXTURE_2D, last_texture);

				m_RedrawFrame = true;
				return true;
			});
	}
	void Texturing::OnImGuiRender ()
	{
		ImGui::Begin (GLCore::ImGuiLayer::UniqueName ("Just a window"));

		if (ImGui::BeginTabBar (GLCore::ImGuiLayer::UniqueName ("Shaders Content"))) {
			if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Settings"))) {

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
			if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Img"))) {

				ImGui::EndTabItem ();
			}
			ImGui::EndTabBar ();
		}
		ImGui::End ();
	}

	void Texturing::OnComputeShaderReload ()
	{
		OnComputeShaderReloadBase ();
	}
	bool Texturing::FillBuffer (GLCore::Timestep delT)
	{
		bool reupload_buffer = false;

		for (uint32_t i = 0; i < m_GeometryData.size (); i++) {
			if (m_GeometryData[i].updated)
				m_GeometryData[i].FillBuffer (&m_GeometryBuff[i]), m_GeometryData[i].updated = (m_GeometryData[i].last_position != m_GeometryData[i].position), reupload_buffer = true;
			m_RedrawFrame |= m_GeometryData[i].updated;
		}

		return reupload_buffer;
	}
}