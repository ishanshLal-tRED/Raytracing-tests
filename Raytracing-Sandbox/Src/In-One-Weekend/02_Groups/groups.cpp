#include "groups.h"
#include <GLCoreUtils.h>
#include <GLCore/Core/KeyCodes.h>
namespace In_One_Weekend
{
	void Groups::OnAttach ()
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

			m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, tex_w, tex_h, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST);

			// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
			glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		}

		if (m_GroupsIDBuffer.first.capacity () < m_NumOfObjInGroup) {
			m_GroupsIDBuffer.first.resize (m_NumOfObjInGroup);
		}
		if (m_GroupsDataBuffer.first.capacity () < m_NumOfObjInGroup) {
			m_GroupsDataBuffer.first.resize (m_NumOfObjInGroup);
		}
		while (m_GeometryGroup.size () < m_NumOfObjInGroup) {
			m_GeometryGroup.emplace_back (Geometry ());
		}
	}
	void Groups::OnDetach ()
	{
		DeleteQuadVAO ();

		glDeleteTextures (1, &m_ComputeShaderOutputTex);
		m_ComputeShaderOutputTex = 0;

		DeleteComputeShader ();
		DeleteSquareShader ();
	}
	void Groups::OnUpdate (GLCore::Timestep ts)
	{
		CopyObjBuffer ();

		glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		{ // launch compute shaders!
			glUseProgram (m_ComputeShaderProgID);

			glUniform1i (m_ShowNormalsUniLoc, int (m_ShowNormals));
			glUniform1i (m_NumOfGeometryUniLoc, m_NumOfObjInGroup);
			glUniform1i (m_Cull_FrontsideUniLoc, int (m_Cull_Frontside));
			glUniform1i (m_Cull_BacksideUniLoc, int (m_Cull_Backside));
			glUniform1i (m_NumOfBouncesUniLoc, m_NumOfBouncesPerRay);
			glUniform1i (m_NumOfSamplesUniLoc, m_NumOfSamplesPerPixel);

			glUniform1f (m_FocusDistUniLoc, m_FocusDist);
			glm::vec3 cam_dirn = FrontFromPitchYaw (m_CameraPitchYaw.x, m_CameraPitchYaw.y);
			glUniform3f (m_CamDirnUniLoc, cam_dirn.x, cam_dirn.y, cam_dirn.z);
			glUniform3f (m_CamPosnUniLoc, m_CameraPosn.x, m_CameraPosn.y, m_CameraPosn.z);

			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_GroupsIDBufferTex);
			glActiveTexture (GL_TEXTURE1);
			glBindTexture (GL_TEXTURE_2D, m_GroupsDataBufferTex);

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
	void Groups::OnEvent (GLCore::Event &event)
	{
		GLCore::EventDispatcher dispatcher (event);
		dispatcher.Dispatch<GLCore::LayerViewportResizeEvent> (
			[&](GLCore::LayerViewportResizeEvent &e) {

				m_OutputTexDimensions.x = (float (e.GetWidth ())/e.GetHeight ())*m_OutputTexDimensions.y;

				glDeleteTextures (1, &m_ComputeShaderOutputTex);
				m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, m_OutputTexDimensions.x, m_OutputTexDimensions.y, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST);

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
						pitch = MIN (pitch + 0.3f, 89.0f); break;
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
	void Groups::OnImGuiRender ()
	{
		using namespace GLCore;
		ImGui::Begin (ImGuiLayer::UniqueName ("Just a window"));
		if (ImGui::BeginTabBar (ImGuiLayer::UniqueName ("Shaders Content"))) {
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Settings"))) {

				ImGui::Text ("Max Compute Work Group\n Count: %d, %d, %d\n Size:  %d, %d, %d", Work_Group_Count[0], Work_Group_Count[1], Work_Group_Count[2], Work_Group_Size[0], Work_Group_Size[1], Work_Group_Size[2]);

				if (ImGui::InputInt ("Resolution Height", &m_OutputResolutionHeight)) {
					int tmp = MIN (MAX (m_OutputResolutionHeight, 100), 1000);
					if (m_OutputTexDimensions.y != tmp)
					{
						m_OutputTexDimensions.x *= (float (tmp)/m_OutputTexDimensions.y);
						m_OutputTexDimensions.y = tmp;
						glDeleteTextures (1, &m_ComputeShaderOutputTex);
						m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, m_OutputTexDimensions.x, m_OutputTexDimensions.y, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST);
						glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
					}
				}

				ImGui::DragFloat3 ("Camera Posn", &m_CameraPosn[0], 0.1f);
				ImGui::DragFloat2 ("Camera pitch(y), yaw(x)", &m_CameraPitchYaw[0]);
				ImGui::DragFloat ("Focus Distance", &m_FocusDist, 0.1f);

				ImGui::Checkbox ("Show Normals", &m_ShowNormals);
				ImGui::SameLine ();
				ImGui::Checkbox ("Cull Front", &m_Cull_Frontside);
				ImGui::SameLine ();
				ImGui::Checkbox ("Cull Back", &m_Cull_Backside);
				
				ImGui::InputInt ("No. of Ray Bounce", &m_NumOfBouncesPerRay);
				ImGui::InputInt ("No. of Samples Per Pixel", &m_NumOfSamplesPerPixel);

				ImGui::Text ("use W, A, S, D to move (while viewport focused)\nand UP, LEFT, DOWN, RIGHT to rotate camera\n");

				if (ImGui::InputInt ("Number Of Object Scene", (int *)((void *)&m_NumOfObjInGroup))) {
					if (m_GroupsIDBuffer.first.capacity () < m_NumOfObjInGroup) {
						m_GroupsIDBuffer.first.resize (m_NumOfObjInGroup);
					}
					if (m_GroupsDataBuffer.first.capacity () < m_NumOfObjInGroup) {
						m_GroupsDataBuffer.first.resize (m_NumOfObjInGroup);
					}
					while (m_GeometryGroup.size () < m_NumOfObjInGroup) {
						m_GeometryGroup.emplace_back (Geometry());
					}
				}
				if (ImGui::CollapsingHeader ("Objects In Scene")) {
					ImGui::Indent ();
					for (uint16_t i = 0; i < m_NumOfObjInGroup; i++) {
						ImGui::PushID (i);
						Geometry &obj = m_GeometryGroup[i];
						ImGui::Combo ("Geometry Type", (int*)&obj.Typ, "None\0Cuboid\0Ellipsoid\0");
						ImGui::InputFloat3 ("Position", &obj.Position[0]);
						if (ImGui::InputFloat3 ("Rotation", &obj.Rotation[0])) obj.ResetInvRotationMatrix ();
						ImGui::InputFloat3 ("Scale", &obj.Scale[0]);
						ImGui::ColorPicker3 ("Color", &obj.Color[0]);
						ImGui::Separator ();
						ImGui::PopID ();
					}
					ImGui::Unindent ();
				}

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
	void Groups::OnComputeShaderReload ()
	{
		m_FocusDistUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_FocusDist");
		m_CamDirnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraDirn");
		m_CamPosnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraPosn");
		m_ShowNormalsUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_ShowNormal");
		m_NumOfGeometryUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfObj");
		m_Cull_FrontsideUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Cull_Front");
		m_Cull_BacksideUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Cull_Back");
		m_NumOfBouncesUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfBounce");
		m_NumOfSamplesUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfSamples");
	}
	void Groups::OnSquareShaderReload ()
	{}

	glm::vec3 Groups::FrontFromPitchYaw (float pitch, float yaw)
	{
		glm::vec3 front;
		front.x = cos (glm::radians (yaw)) * cos (glm::radians (pitch));
		front.y = sin (glm::radians (pitch));
		front.z = sin (glm::radians (yaw)) * cos (glm::radians (pitch));
		return glm::normalize (front);
	}
	void Groups::CopyObjBuffer ()
	{
		//bool resize_ID_tex = false, resize_data_tex = false;
		while (m_GroupsIDBuffer.first.size () < m_GeometryGroup.size ())
			m_GroupsIDBuffer.first.push_back (0.0f), m_GroupsIDBuffer.second = true;// , resize_ID_tex = true;
		while (m_GroupsDataBuffer.first.size () < m_GeometryGroup.size ())
			m_GroupsDataBuffer.first.emplace_back (GeometryBuff ()), m_GroupsDataBuffer.second = true;// , resize_data_tex = true;
		for (uint16_t i = 0; i < m_GeometryGroup.size (); i++) {
			m_GroupsDataBuffer.second |= m_GeometryGroup[i].FillBuffer (m_GroupsDataBuffer.first[i]);
		}
		for (uint16_t i = 0; i < m_GeometryGroup.size (); i++) {
			if (m_GroupsIDBuffer.first[i] != int(m_GeometryGroup[i].Typ))
				m_GroupsIDBuffer.first[i] = float (m_GeometryGroup[i].Typ), m_GroupsIDBuffer.second = true;
		}

		if (m_GroupsDataBuffer.second) {
			//if (resize_data_tex || m_GroupsDataBuffer.second) {
				glDeleteTextures (1, &m_GroupsDataBufferTex);
				m_GroupsDataBufferTex = 0;
			//}
			if (m_GroupsDataBufferTex == 0) {
				m_GroupsDataBufferTex = Helper::TEXTURE_2D::Upload (m_GroupsDataBuffer.first.data (), sizeof (GeometryBuff)/sizeof (float[3]), m_GroupsDataBuffer.first.size (), GL_RGB32F, GL_RGB, GL_FLOAT);
			}
			m_GroupsDataBuffer.second = false;
		}
		if (m_GroupsIDBuffer.second) {
			//if (resize_ID_tex || m_GroupsIDBuffer.second) { // currently i'm recreating texture as a change occurs in data it instead of changing data texture
				glDeleteTextures (1, &m_GroupsIDBufferTex);
				m_GroupsIDBufferTex = 0;
			//}
			if (m_GroupsIDBufferTex == 0) {
				m_GroupsIDBufferTex = Helper::TEXTURE_2D::Upload (m_GroupsIDBuffer.first.data (), m_GroupsIDBuffer.first.size (), 1, GL_R32F, GL_RED, GL_FLOAT);
			}
			m_GroupsIDBuffer.second = false;
		}
	}
};