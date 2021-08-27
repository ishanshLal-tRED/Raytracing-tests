#include "lights.h"
#include <GLCore/Core/KeyCodes.h>
#include <fstream>
#include <filesystem>

namespace In_Next_Week
{
	uint32_t GeometryData_04::MaxTextureBindSlots = 6; // slots_already_assigned = 2;
	uint32_t GeometryData_04::OffsetBindSlots = 2;
	std::vector<std::pair<GLuint, uint32_t>> GeometryData_04::AllTextures;
	std::vector<char> GeometryData_04::AllTexturesStr = {'N', 'o', 'n', 'e', '\0', '\0'};
	
	static uint32_t num_of_textures = 0;

	void Lights::OnUpdate (GLCore::Timestep ts)
	{
		{
			GeometryData_04::OffsetBindSlots = 2; // slots_already_assigned = 2;
			GeometryData_04::MaxTextureBindSlots = m_MaxTextureBindSlots; // me being lazy: Update for every instance

			OnUpdateBase (ts, [&]() {
				GeometryData_04::BindExtraData ();

				glUniform1ui (m_UniformIDNumOfLightSources, m_LightSSBOBuff.size ());
				
				glBindBuffer (GL_SHADER_STORAGE_BUFFER, m_ComputeShaderInput_SSBOLights);
				glBindBufferBase (GL_SHADER_STORAGE_BUFFER, SSBO_LightIdentifiers, m_ComputeShaderInput_SSBOLights);
				
				if(LastIterationSSBOSize == m_LightSSBOBuff.size ())
					glBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, sizeof (GeometryData_04::LightClass)*m_LightSSBOBuff.size (), m_LightSSBOBuff.data ());
				else glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GeometryData_04::LightClass)*m_LightSSBOBuff.size (), m_LightSSBOBuff.data (), GL_DYNAMIC_COPY);
				uint32_t i = 1;
				for (GeometryData_04::LightClass &ref : m_LightSSBOBuff) {
					LOG_TRACE (" #{0} BB[({1}, {2}, {3}) ({4}, {5}, {6})]({7})", i++, ref.BBmin[0], ref.BBmin[1], ref.BBmin[2], ref.BBmax[0], ref.BBmax[1], ref.BBmax[2], ref.idx);
				}
			});
		}
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
	void Lights::OnAttach ()
	{
		m_NumberOfGeometriesToRender = 5;

		OnAttachBase ();
		static bool first = true;
		if (first) {
			first = false;
			GeometryData_04::AddTextureOption ("assets/dice.png");
		}

		LOG_TRACE ("GeomBuffSize: {0}, SceneNodeSize: {1}", sizeof (GeometryBuff_04), sizeof (LBVH::BVHNodeBuff));
		// Setup
		m_Camera.Position = glm::vec3 (-3, 6.5f, -3);
		m_Camera.pitch = -30;
		m_Camera.yaw = 45;

		if (m_ComputeShaderInput_SSBOLights == 0)
		{
			glGenBuffers (1, &m_ComputeShaderInput_SSBOLights);
			glBindBuffer (GL_SHADER_STORAGE_BUFFER, m_ComputeShaderInput_SSBOLights);
			GeometryData_04::LightClass dummy;
			glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (dummy), &dummy, GL_DYNAMIC_COPY);
			glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0); // unbind
		}
	}
	void Lights::OnDetach ()
	{
		OnDetachBase ();

		if(m_ComputeShaderInput_SSBOLights != 0)
			glDeleteBuffers (1, &m_ComputeShaderInput_SSBOLights);
		m_ComputeShaderInput_SSBOLights = 0;
	}
	void Lights::OnEvent (GLCore::Event &event)
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
	void Lights::OnImGuiRender ()
	{
		ImGui::Begin (GLCore::ImGuiLayer::UniqueName ("Just a window"));

		if (ImGui::BeginTabBar (GLCore::ImGuiLayer::UniqueName ("Shaders Content"))) {
			if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Settings"))) {
				if (ImGui::Button ("Set Configration"))
				{
					m_GeometryData[1].position = glm::vec3 (3);
					m_GeometryData[0].isEmissive = true;
					m_GeometryData[0].updated = true;
					
					m_GeometryData[1].position = glm::vec3 (-1.52f, 3.0f, 1.4f);
					m_GeometryData[1].isEmissive = true;
					m_GeometryData[1].updated = true;

					m_GeometryData[2].position = glm::vec3 (9.0f, 0.0f, 9.0f);
					m_GeometryData[2].Color = glm::vec3 (0.3, 0.4, 1.0);
					m_GeometryData[2].updated = true;

					m_GeometryData[3].position = glm::vec3 (7.0f, 0.0f, 8.0f);
					m_GeometryData[3].Color = glm::vec3 (0.3, 0.4, 1.0);
					m_GeometryData[3].updated = true;

					m_GeometryData[4].position = glm::vec3(0.0f, -2.0f, 0.0f);
					m_GeometryData[4].scale = glm::vec3(100,1, 100);
					m_GeometryData[4].Color = glm::vec3 (0.3, 0.4, 1.0);
					m_GeometryData[4].Refractivity = 0.1f;
					m_GeometryData[4].Reflectivity = 0.6f;
					m_GeometryData[4].Type = GeometryType_04::Cuboid;
					m_GeometryData[4].updated = true;

					m_Camera.Position.x = -6.1;
					m_Camera.Position.y =  6.2;
					m_Camera.Position.z = -0.2;
					m_RedrawFrame = true;

				}ImGui::SameLine ();
				OnImGuiRenderBase ();

				if (ImGui::InputInt ("Number Of Texture Bind Slots", &m_MaxTextureBindSlots)) {
					GLint max_tex_bind_slots;
					glGetIntegerv (GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &max_tex_bind_slots);
					int slots_already_assigned = 2; // for geomBuff & SceneHier
					
					auto tmp = m_MaxTextureBindSlots;
					m_MaxTextureBindSlots = MAX (1, MIN (max_tex_bind_slots - slots_already_assigned, m_MaxTextureBindSlots));
					if (tmp != m_MaxTextureBindSlots)
					{
						FindAndSetStr (m_ComputeShaderTXT, { "#define","u_NumOfTexture2D" }, std::to_string (m_MaxTextureBindSlots));
					}
				}

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
			if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Additional Material settings"))) {
				static GLuint display_texture = 0;
				static int selected = 0;
				ImGui::Text ("If you cannot see your uploaded texture here reselect from combo");
				ImGui::Image ((void *)(display_texture), ImVec2 (ImGui::GetContentRegionAvailWidth (), ImGui::GetContentRegionAvailWidth () / 4), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				if (ImGui::Combo ("already uploaded textures", &selected, GeometryData_04::AllTexturesStr.data ())) {
					display_texture = GeometryData_04::AllTextures[selected - 1].first;
				}
				ImGui::Separator ();
				static int selectedTYP = 0;
					static float freq = 0.01f, lac = 2.5f, gain = 0.5f;
					static int octaves = 5;
					switch (selectedTYP) {
						case 0: // simplex
							ImGui::InputFloat ("Downscale", &freq);
							break;
						case 1: // FBM
						case 2: // Turbulance
							ImGui::InputFloat ("Frequency", &freq);
							ImGui::SameLine ();
							ImGui::InputFloat ("Lacunarity", &lac);
							//ImGui::SameLine ();
							ImGui::InputFloat ("Gain", &gain);
							ImGui::SameLine ();
							ImGui::InputInt ("Octaves", &octaves);
							break;
					}
				ImGui::Combo ("TYPE", &selectedTYP, "Simplex Noise\0FRACTAL BROWNIM MOTION\0TURBULANCE\0"); ImGui::SameLine ();
				if (ImGui::Button ("Add Another Texture")) {
					GLuint texture = Helper::Noise::MakeTexture<glm::vec3> (600, 100, Helper::Noise::NOISE_TYP(selectedTYP), { glm::vec3 (0), glm::vec3 (1) }, freq, lac, gain, octaves);
					std::string textureName;
					switch (selectedTYP)
					{
					case 0: // simplex
						textureName = "Simplex_Noise_" + std::to_string (texture);
						GeometryData_04::AddTextureOption (texture, textureName);
						break;
					case 1: // FBM
						textureName = "FBM2_" + std::to_string (texture);
						GeometryData_04::AddTextureOption (texture, textureName);
						break;
					case 2: // Turbulance
						textureName = "Turbulance_" + std::to_string (texture);
						GeometryData_04::AddTextureOption (texture, textureName);
						break;
					}
					display_texture = texture;
					selected = GeometryData_04::AllTextures.size ();
				}
				ImGui::Separator ();
				static int imprtFrmt = 0;
				if (ImGui::Button ("Add From Disk")) {
					display_texture = GeometryData_04::AddTextureOption (GLCore::Utils::FileDialogs::OpenFile ("Image\0*.jpg\0*.png\0*.jpeg\0*.bmp\0*.hdr\0*.psd\0*.tga\0*.gif\0*.pic\0*.psd\0*.pgm\0").c_str (), Helper::TEXTURE_2D::MAPPING (imprtFrmt));
					selected = GeometryData_04::AllTextures.size ();
				}
				ImGui::Combo ("Import Mapping Format", &imprtFrmt, "Cubic Projection\0Mercator Projection\0");
				if (ImGui::IsItemActivated() || ImGui::IsItemHovered ())
				{
					ImGui::BeginTooltip ();
					ImGui::TextUnformatted ("FOR MERCATOR: u, v coordinates ∈ [0 1] where u goes from -Y to +Y & v goes from -X to +Z to +X to -Z back to -X\nFOR CUBIC(custom format): x = face + _x; _x, y ∈ [0 1] where face: [+Y -> +X -> +Z -> -X -> -Z -> -Y]");
					ImGui::EndTooltip ();
				}

				ImGui::EndTabItem ();
			}
			ImGui::EndTabBar ();
		}
		ImGui::End ();
	}

	void Lights::OnComputeShaderReload ()
	{
		OnComputeShaderReloadBase ();
		m_UniformIDNumOfLightSources = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfLightSources");

		GLuint blockIndex = glGetProgramResourceIndex (m_ComputeShaderProgID, GL_SHADER_STORAGE_BLOCK, "sbo_Light");
		glShaderStorageBlockBinding (m_ComputeShaderProgID, blockIndex, SSBO_LightIdentifiers);
	}
	bool Lights::FillBuffer (GLCore::Timestep delT)
	{
		bool reupload_buffer = false;
		LastIterationSSBOSize = m_LightSSBOBuff.size ();
		m_LightSSBOBuff.clear ();
		for (uint32_t i = 0; i < m_GeometryData.size (); i++) {
			if (m_GeometryData[i].isEmissive) {
				auto [min, max] = m_GeometryData[i].CalculateBBMinMax ();
				m_LightSSBOBuff.push_back ({ min[0],min[1],min[2],max[0],max[1],max[2], GLuint(i)});
			}
			if (m_GeometryData[i].updated)
				m_GeometryData[i].FillBuffer (&m_GeometryBuff[i]), m_GeometryData[i].updated = (m_GeometryData[i].last_position != m_GeometryData[i].position), reupload_buffer = true;
			m_RedrawFrame |= m_GeometryData[i].updated;
		}

		return reupload_buffer;
	}
}