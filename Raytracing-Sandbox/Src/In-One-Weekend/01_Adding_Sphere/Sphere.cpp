#include "Sphere.h"
#include <GLCoreUtils.h>
#include <GLCore/Core/KeyCodes.h>
namespace In_One_Weekend
{
	void Sphere::OnAttach ()
	{
		GLCore::Utils::EnableGLDebugging ();

		glEnable (GL_DEPTH_TEST);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		ReloadComputeShader (); ReloadSquareShader ();
		ReGenQuadVAO ();
		{
			int *work_grp_cnt = (int *)((void *)(&Work_Group_Count[0]));
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

			int *work_grp_size = (int *)((void *)(&Work_Group_Size[0]));
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
		}
		if (!m_ComputeShaderOutputTex) {
			// dimensions of the image
			int tex_w = m_OutputTexDimensions.x, tex_h = m_OutputTexDimensions.y;

			m_ComputeShaderOutputTex = m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, tex_w, tex_h, GL_RGBA32F, GL_RGBA, GL_FLOAT);

			// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
			glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		}
	}
	void Sphere::OnDetach ()
	{
		DeleteQuadVAO ();

		glDeleteTextures (1, &m_ComputeShaderOutputTex);
		m_ComputeShaderOutputTex = 0;

		DeleteComputeShader ();
		DeleteSquareShader ();
	}
	void Sphere::OnUpdate (GLCore::Timestep ts)
	{
		glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		{ // launch compute shaders!
			glUseProgram (m_ComputeShaderProgID);

			glUniform1i (m_ShowNormalsUniLoc, int(m_ShowNormals));
			glUniform3f (m_SpherePosnUniLoc, m_Sphere.x, m_Sphere.y, m_Sphere.z);
			glUniform1f (m_SphereRadiusUniLoc, m_Sphere.w);

			glUniform1f (m_FocusDistUniLoc, m_FocusDist);
			glm::vec3 cam_dirn = FrontFromPitchYaw (m_CameraPitchYaw.x, m_CameraPitchYaw.y);
			glUniform3f (m_CamDirnUniLoc, cam_dirn.x, cam_dirn.y, cam_dirn.z);
			glUniform3f (m_CamPosnUniLoc, m_CameraPosn.x, m_CameraPosn.y, m_CameraPosn.z);
			
			glDispatchCompute (m_OutputTexDimensions.x, m_OutputTexDimensions.y, 1);
		}

		// make sure writing to image has finished before read
		glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		{ // normal drawing pass
			glClear (GL_COLOR_BUFFER_BIT);
			glUseProgram (m_SquareShaderProgID);
			glBindVertexArray (m_QuadVA);
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputTex);
			glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		}
	}
	void Sphere::OnEvent (GLCore::Event &event)
	{
		GLCore::EventDispatcher dispatcher (event);
		dispatcher.Dispatch<GLCore::LayerViewportResizeEvent> (
			[&](GLCore::LayerViewportResizeEvent &e) {
				glDeleteTextures (1, &m_ComputeShaderOutputTex);

				m_OutputTexDimensions.x = (float(e.GetWidth ())/e.GetHeight ())*100, m_OutputTexDimensions.y = 100;
				
				m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, m_OutputTexDimensions.x, m_OutputTexDimensions.y, GL_RGBA32F, GL_RGBA, GL_FLOAT);
				
				// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
				glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

				return false;
			});
		dispatcher.Dispatch<GLCore::KeyPressedEvent> (
			[&](GLCore::KeyPressedEvent &e) {
				float &pitch = m_CameraPitchYaw.x;
				float &yaw = m_CameraPitchYaw.y;
				glm::vec3 front = FrontFromPitchYaw (pitch, yaw), right, up;
				
				static constexpr glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
				right = glm::normalize (glm::cross (front, worldUp));
				// up = glm::normalize (glm::cross (right, front));

				switch (e.GetKeyCode ()) {
					case GLCore::Key::Up:
						pitch = MIN (pitch + 0.3f,  89.0f); break;
					case GLCore::Key::Down:
						pitch = MAX (pitch - 0.3f, -89.0f); break;
					case GLCore::Key::Left:
						yaw = MOD (yaw - 0.3f, 360.0f); break;
					case GLCore::Key::Right:
						yaw = MOD (yaw + 0.3f, 360.0f); break;
					case GLCore::Key::W:
						m_CameraPosn += front*0.1f; break;
					case GLCore::Key::S:
						m_CameraPosn -= front*0.1f; break;
					case GLCore::Key::A:
						m_CameraPosn -= right*0.1f; break;
					case GLCore::Key::D:
						m_CameraPosn += right*0.1f; break;
				}

				return false;
			});
	}
	void Sphere::OnImGuiRender ()
	{
		using namespace GLCore;
		ImGui::Begin (ImGuiLayer::UniqueName ("Just a window"));
		if (ImGui::BeginTabBar (ImGuiLayer::UniqueName ("Shaders Content"))) {
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Settings"))) {

				ImGui::Text ("Max Compute Work Group\n Count: %d, %d, %d\n Size:  %d, %d, %d", Work_Group_Count[0], Work_Group_Count[1], Work_Group_Count[2], Work_Group_Size[0], Work_Group_Size[1], Work_Group_Size[2]);
				
				ImGui::DragFloat3 ("Camera Posn", &m_CameraPosn[0], 0.1f);
				ImGui::DragFloat2 ("Camera pitch(y), yaw(x)", &m_CameraPitchYaw[0]);
				ImGui::DragFloat ("Focus Distance", &m_FocusDist, 0.1f);
				
				ImGui::Checkbox ("Show Sphere Normal", &m_ShowNormals);
				
				ImGui::DragFloat3 ("Sphere Position", &m_Sphere[0]);
				ImGui::DragFloat ("Sphere Radius", &m_Sphere[3]);

				ImGui::Text ("use W, A, S, D to move (while viewport focused)\nand UP, LEFT, DOWN, RIGHT to rotate camera\n");
				
				ImGui::EndTabItem ();
			}
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Compute Shader Source"))) {

				OnImGuiComputeShaderSource ();

				ImGui::EndTabItem ();
			}
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Square Shader Source"))) {

				OnImGuiSqureShaderSource ();

				ImGui::EndTabItem ();
			}
			ImGui::EndTabBar ();
		}
		ImGui::End ();
	}
	void Sphere::OnComputeShaderReload ()
	{
		m_FocusDistUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_FocusDist");
		m_CamDirnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraDirn");
		m_CamPosnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraPosn");
		m_ShowNormalsUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_ShowNormal");
		m_SphereRadiusUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Sphere.Radius");
		m_SpherePosnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Sphere.Centre");
	}
	void Sphere::OnSquareShaderReload ()
	{}

	glm::vec3 Sphere::FrontFromPitchYaw (float pitch, float yaw)
	{
		glm::vec3 front;
		front.x = cos (glm::radians (yaw)) * cos (glm::radians (pitch));
		front.y = sin (glm::radians (pitch));
		front.z = sin (glm::radians (yaw)) * cos (glm::radians (pitch));
		return glm::normalize (front);
	}
};