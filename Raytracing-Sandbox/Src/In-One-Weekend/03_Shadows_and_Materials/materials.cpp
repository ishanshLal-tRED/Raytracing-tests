#include "materials.h"
#include <GLCoreUtils.h>
#include <GLCore/Core/KeyCodes.h>
namespace In_One_Weekend
{
	void Adding_Materials::OnAttach ()
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
		{// pre-constructing a scene
			m_GeometryGroup[0].Typ = Geom_type::ELLIPSOID;
			m_GeometryGroup[0].Color = glm::vec3(0);
			m_GeometryGroup[0].Scale = glm::vec3 (2);
			m_GeometryGroup[0].Position.y = 1;
			m_GeometryGroup[0].Material = glm::vec3(1, 0, 0);
			m_GeometryGroup[1].Typ = Geom_type::CUBOID;
			m_GeometryGroup[1].Position.z = -2;
			m_GeometryGroup[1].Position.y = -1.5f;
			m_GeometryGroup[1].Scale.x = 10;
			m_GeometryGroup[1].Scale.z = 10;
			m_GeometryGroup[1].Color = glm::vec3 (0.02, 0.0125, 0.08);
			m_GeometryGroup[2].Typ = Geom_type::CUBOID;
			m_GeometryGroup[2].Position.x = -1;
			m_GeometryGroup[2].Position.y = -.75f;
			m_GeometryGroup[2].Position.z = 1.5f;
			m_GeometryGroup[2].Scale = glm::vec3(.5f);
		}
	}
	void Adding_Materials::OnDetach ()
	{
		DeleteQuadVAO ();

		glDeleteTextures (1, &m_ComputeShaderOutputTex);
		m_ComputeShaderOutputTex = 0;

		DeleteComputeShader ();
		DeleteSquareShader ();
	}
	void Adding_Materials::OnUpdate (GLCore::Timestep ts)
	{
		glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (m_UpdateFrame) { // launch compute shaders!
			CopyObjBuffer ();
			m_UpdateFrame = false;
			m_MaxTileIndexs[0] = (m_OutputTexDimensions.x/m_TileSize.x), m_MaxTileIndexs[1] = (m_OutputTexDimensions.y/m_TileSize.y);
			m_TileIndexs[0] = (m_MaxTileIndexs[0]-0.1f)/2, m_TileIndexs[1] = (m_MaxTileIndexs[1]-0.1f)/2;

			m_TileRingLimit00[0] =   m_TileIndexs[0];
			m_TileRingLimit00[1] =   m_TileIndexs[1];
			m_TileRingLimit01[0] =   m_TileIndexs[0];
			m_TileRingLimit01[1] =   m_TileIndexs[1] + 1;
			m_TileRingLimit10[0] =   m_TileIndexs[0] + 1;
			m_TileRingLimit10[1] =   m_TileIndexs[1] - 1; // for next ring
			m_TileRingLimit11[0] =   m_TileIndexs[0] + 1;
			m_TileRingLimit11[1] =   m_TileIndexs[1] + 1;
			m_TileIndexLastStep[0] = 0;
			m_TileIndexLastStep[1] = 0;
		}
		for (uint8_t i = 0; i < m_NumberOfTilesAtATime; i++) {
		DRAW_TILE:
			if (MIN (m_TileIndexs[0], m_TileIndexs[1]) < MAX (m_MaxTileIndexs[0], m_MaxTileIndexs[1]) + 1) {
				if (m_TileIndexs[1] == m_TileRingLimit00[1] && m_TileIndexs[0] == m_TileRingLimit00[0]) {
					m_TileRingLimit00[0]--, m_TileRingLimit00[1]--;
					m_TileIndexLastStep[0] = 0, m_TileIndexLastStep[1] = 1;
				}
				if (m_TileIndexs[1] == m_TileRingLimit01[1] && m_TileIndexs[0] == m_TileRingLimit01[0]) {
					m_TileRingLimit01[0]--, m_TileRingLimit01[1]++;
					m_TileIndexLastStep[0] = 1, m_TileIndexLastStep[1] = 0;
				}
				if (m_TileIndexs[1] == m_TileRingLimit11[1] && m_TileIndexs[0] == m_TileRingLimit11[0]) {
					m_TileRingLimit11[0]++, m_TileRingLimit11[1]++;
					m_TileIndexLastStep[0] = 0, m_TileIndexLastStep[1] = -1;
				}
				if (m_TileIndexs[1] == m_TileRingLimit10[1] && m_TileIndexs[0] == m_TileRingLimit10[0]) {
					m_TileRingLimit10[0]++, m_TileRingLimit10[1]--;
					m_TileIndexLastStep[0] = -1, m_TileIndexLastStep[1] = 0;
				}
				bool drawableTile = m_TileIndexs[1] > -1 && m_TileIndexs[1] < m_MaxTileIndexs[1]+1 && m_TileIndexs[0] > -1 && m_TileIndexs[0] < m_MaxTileIndexs[0]+1;
				if (drawableTile) {
					glUseProgram (m_ComputeShaderProgID);

					glUniform1i (m_ShowNormalsUniLoc, int (m_ShowNormals));
					//glUniform1i (m_DiffusedUniLoc, int (m_Diffused));
					glUniform1i (m_NumOfGeometryUniLoc, m_NumOfObjInGroup);
					//glUniform1i (m_Cull_FrontsideUniLoc, int (m_Cull_Frontside));
					//glUniform1i (m_Cull_BacksideUniLoc, int (m_Cull_Backside));
					glUniform1i (m_NumOfBouncesUniLoc, m_NumOfBouncesPerRay);
					glUniform1i (m_NumOfSamplesUniLoc, m_NumOfSamplesPerPixel);

					glUniform2i (m_TileIndexsLocation, m_TileIndexs[0], m_TileIndexs[1]);
					glUniform2i (m_TileSizeLocation, m_TileSize.x, m_TileSize.y);

					glUniform1f (m_FocusDistUniLoc, m_FocusDist);
					glm::vec3 cam_dirn = FrontFromPitchYaw (m_CameraPitchYaw.x, m_CameraPitchYaw.y);
					glUniform3f (m_CamDirnUniLoc, cam_dirn.x, cam_dirn.y, cam_dirn.z);
					glUniform3f (m_CamPosnUniLoc, m_CameraPosn.x, m_CameraPosn.y, m_CameraPosn.z);

					glActiveTexture (GL_TEXTURE0);
					glBindTexture (GL_TEXTURE_2D, m_GroupsIDBufferTex);
					glActiveTexture (GL_TEXTURE1);
					glBindTexture (GL_TEXTURE_2D, m_GroupsDataBufferTex);

					glDispatchCompute ((m_TileIndexs[0] >= m_MaxTileIndexs[0] ? (m_OutputTexDimensions.x%m_TileSize.x) : m_TileSize.x)
									   , (m_TileIndexs[1] >= m_MaxTileIndexs[1] ? (m_OutputTexDimensions.y%m_TileSize.y) : m_TileSize.y), 1);

					// make sure writing to image has finished before read
					glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				}
				m_TileIndexs[0] += m_TileIndexLastStep[0], m_TileIndexs[1] += m_TileIndexLastStep[1];
				if (!drawableTile)
					goto DRAW_TILE;
			}
		}

		{ // normal drawing pass
			glClear (GL_COLOR_BUFFER_BIT);
			glUseProgram (m_SquareShaderProgID);
			glBindVertexArray (m_QuadVA);
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputTex);
			glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		}
	}
	void Adding_Materials::OnEvent (GLCore::Event &event)
	{
		GLCore::EventDispatcher dispatcher (event);
		dispatcher.Dispatch<GLCore::LayerViewportResizeEvent> (
			[&](GLCore::LayerViewportResizeEvent &e) {

				m_OutputTexDimensions.x = (float (e.GetWidth ())/e.GetHeight ())*m_OutputTexDimensions.y;

				glDeleteTextures (1, &m_ComputeShaderOutputTex);
				m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, m_OutputTexDimensions.x, m_OutputTexDimensions.y, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST);

				// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
				glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

				m_UpdateFrame = true;
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
						pitch = MIN (pitch + 0.3f, 89.0f); m_UpdateFrame = true; break;
					case GLCore::Key::Down:
						pitch = MAX (pitch - 0.3f, -89.0f); m_UpdateFrame = true; break;
					case GLCore::Key::Left:
						yaw = MOD (yaw - 0.3f, 360.0f); m_UpdateFrame = true; break;
					case GLCore::Key::Right:
						yaw = MOD (yaw + 0.3f, 360.0f); m_UpdateFrame = true; break;
					case GLCore::Key::W:
						m_CameraPosn += front*0.1f; m_UpdateFrame = true; break;
					case GLCore::Key::S:
						m_CameraPosn -= front*0.1f; m_UpdateFrame = true; break;
					case GLCore::Key::A:
						m_CameraPosn -= right*0.1f; m_UpdateFrame = true; break;
					case GLCore::Key::D:
						m_CameraPosn += right*0.1f; m_UpdateFrame = true; break;
				}

				return false;
			});
	}
	void Adding_Materials::OnImGuiRender ()
	{
		using namespace GLCore;
		ImGui::Begin (ImGuiLayer::UniqueName ("Just a window"));
		if (ImGui::BeginTabBar (ImGuiLayer::UniqueName ("Shaders Content"))) {
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Settings"))) {

				ImGui::Text ("Max Compute Work Group\n Count: %d, %d, %d\n Size:  %d, %d, %d", Work_Group_Count[0], Work_Group_Count[1], Work_Group_Count[2], Work_Group_Size[0], Work_Group_Size[1], Work_Group_Size[2]);
				ImGui::InputInt ("Number of tiles to draw at a time, increase it for stronger GPUs", &m_NumberOfTilesAtATime);
				ImGui::InputInt2 ("Size of tiles, increase it for stronger GPUs", (int*)((void*)&m_TileSize));
				if (ImGui::InputInt ("Resolution Height", &m_OutputResolutionHeight)) {
					int tmp = MIN (MAX (m_OutputResolutionHeight, 100), 1000);
					if (m_OutputTexDimensions.y != tmp)
					{
						m_OutputTexDimensions.x *= (float (tmp)/m_OutputTexDimensions.y);
						m_OutputTexDimensions.y = tmp;
						glDeleteTextures (1, &m_ComputeShaderOutputTex);
						m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, m_OutputTexDimensions.x, m_OutputTexDimensions.y, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST);
						glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

						m_UpdateFrame = true;
					}
				}

				m_UpdateFrame |= ImGui::DragFloat3 ("Camera Posn", &m_CameraPosn[0], 0.1f);
				m_UpdateFrame |= ImGui::DragFloat2 ("Camera pitch(y), yaw(x)", &m_CameraPitchYaw[0]);
				m_UpdateFrame |= ImGui::DragFloat ("Focus Distance", &m_FocusDist, 0.1f);

				m_UpdateFrame |= ImGui::Checkbox ("Show Normals", &m_ShowNormals);

				//ImGui::SameLine ();
				//m_UpdateFrame |= ImGui::Checkbox ("Diffused", &m_Diffused);
				//m_UpdateFrame |= ImGui::Checkbox ("Cull Front", &m_Cull_Frontside);
				//ImGui::SameLine ();
				//m_UpdateFrame |= ImGui::Checkbox ("Cull Back", &m_Cull_Backside);
				
				m_UpdateFrame |= ImGui::InputInt ("No. of Ray Bounce", &m_NumOfBouncesPerRay);
				m_UpdateFrame |= ImGui::InputInt ("No. of Samples Per Pixel", &m_NumOfSamplesPerPixel);

				ImGui::Text ("use W, A, S, D to move (while viewport focused)\nand UP, LEFT, DOWN, RIGHT to rotate camera\n");

				if (ImGui::InputInt ("Number Of Object Scene", (int *)((void *)&m_NumOfObjInGroup))) {
					if (m_GroupsIDBuffer.first.capacity () < m_NumOfObjInGroup) {
						m_GroupsIDBuffer.first.resize (m_NumOfObjInGroup);
						m_UpdateFrame = true;
					}
					if (m_GroupsDataBuffer.first.capacity () < m_NumOfObjInGroup) {
						m_GroupsDataBuffer.first.resize (m_NumOfObjInGroup);
						m_UpdateFrame = true;
					}
					while (m_GeometryGroup.size () < m_NumOfObjInGroup) {
						m_GeometryGroup.emplace_back (Geometry());
						m_UpdateFrame = true;
					}
				}
				if (ImGui::CollapsingHeader ("Objects In Scene")) {
					ImGui::Indent ();
					for (uint16_t i = 0; i < m_NumOfObjInGroup; i++) {
						ImGui::PushID (i);
						Geometry &obj = m_GeometryGroup[i];
						m_UpdateFrame |= ImGui::Combo ("Geometry Type", (int*)&obj.Typ, "None\0Cuboid\0Ellipsoid\0");
						m_UpdateFrame |= ImGui::InputFloat3 ("Position", &obj.Position[0]);
						if (ImGui::InputFloat3 ("Rotation", &obj.Rotation[0])) obj.ResetInvRotationMatrix (), m_UpdateFrame = true;
						m_UpdateFrame |= ImGui::InputFloat3 ("Scale", &obj.Scale[0]);
						m_UpdateFrame |= ImGui::DragFloat3 ("Material(refractivity, reflectivity, scatteritivity)", &obj.Material[0], 0.005f, 0.0f, 1.0f);
						m_UpdateFrame |= ImGui::ColorPicker3 ("Color", &obj.Color[0]);
						ImGui::Text ("{%f, %f, %f}", obj.Color.x, obj.Color.y, obj.Color.z);
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
	void Adding_Materials::OnComputeShaderReload ()
	{
		m_FocusDistUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_FocusDist");
		m_CamDirnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraDirn");
		m_CamPosnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraPosn");
		m_ShowNormalsUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_ShowNormal");
		m_NumOfGeometryUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfObj");
		//m_Cull_FrontsideUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Cull_Front");
		//m_Cull_BacksideUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Cull_Back");
		m_NumOfBouncesUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfBounce");
		m_NumOfSamplesUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfSamples");
		//m_DiffusedUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Diffuse");
		m_TileSizeLocation = glGetUniformLocation (m_ComputeShaderProgID, "u_TileSize");
		m_TileIndexsLocation = glGetUniformLocation (m_ComputeShaderProgID, "u_TileIndex");
	}
	void Adding_Materials::OnSquareShaderReload ()
	{}

	glm::vec3 Adding_Materials::FrontFromPitchYaw (float pitch, float yaw)
	{
		glm::vec3 front;
		front.x = cos (glm::radians (yaw)) * cos ( glm::radians (pitch));
		front.y = sin (glm::radians (pitch));
		front.z = sin (glm::radians (yaw)) * cos (glm::radians (pitch));
		return glm::normalize (front);
	}
	void Adding_Materials::CopyObjBuffer ()
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