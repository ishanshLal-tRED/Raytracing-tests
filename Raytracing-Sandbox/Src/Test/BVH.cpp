﻿#include "BVH.h"

namespace In_Next_Week
{
	const char *BVH::s_FragShaderSrc = R"(
		#version 440 core
		layout (location = 0) in vec2 b_Coord;
		
		layout (r32f, binding = 0) uniform sampler2D u_DptTexture;
		layout (rgba32f, binding = 1) uniform sampler2D u_Texture;

		layout (location = 0) out vec4 o_Color;

		uniform bool u_UseDpthTexture;

		void main ()
		{
			if(!u_UseDpthTexture)
				o_Color = texture (u_Texture, b_Coord);
			else{
				float temp = texture (u_DptTexture, b_Coord).r;
				o_Color = vec4(temp, temp, temp, 1.0);
			}
		})";
	void BVH::OnUpdate (GLCore::Timestep ts)
	{
		OnUpdateBase (ts);
		
		{ // normal drawing pass
			glClear (GL_COLOR_BUFFER_BIT);
			glUseProgram (m_SquareShaderProgID);
			glBindVertexArray (m_QuadVA);

			glUniform1i (m_BoolUseDepthTextureUniformID, int (m_UseDepthTexture));
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputDepthTextureID);
			glActiveTexture (GL_TEXTURE1);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputColorTextureID);
			
			glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		}
	}
	void BVH::OnAttach ()
	{
		OnAttachBase ();

		// Setup
		m_Camera.Position = glm::vec3 (-3, 6.5f, -3);
		m_Camera.pitch = -30;
		m_Camera.yaw = 45;
	}
	void BVH::OnDetach ()
	{
		OnDetachBase ();
	}
	void BVH::OnEvent (GLCore::Event &event)
	{
		GLCore::EventDispatcher dispatcher(event);
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
	void BVH::OnImGuiRender ()
	{
		ImGui::Begin (GLCore::ImGuiLayer::UniqueName ("Just a window"));
		
		if (ImGui::BeginTabBar (GLCore::ImGuiLayer::UniqueName ("Shaders Content")))
		{
			if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Settings"))) {
				ImGui::Checkbox ("Show Depth Texture", &m_UseDepthTexture);
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
	{
		m_BoolUseDepthTextureUniformID = glGetUniformLocation (m_SquareShaderProgID, "u_UseDpthTexture");
	}
	bool BVH::FillBuffer (GLCore::Timestep delT)
	{
		bool reupload_buffer = false;

		for (uint32_t i = 0; i < m_GeometryData.size (); i++)
			if (m_GeometryData[i].updated)
				m_GeometryData[i].FillBuffer (&m_GeometryBuff[i]), m_GeometryData[i].updated = false, reupload_buffer = true;

		return reupload_buffer;
	}
}