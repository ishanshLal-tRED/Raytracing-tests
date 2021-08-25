#include "texturing.h"
#include <GLCore/Core/KeyCodes.h>
#include <fstream>
#include <filesystem>

namespace In_Next_Week
{
	std::vector<GLuint> GeometryData_03::AllTextures = std::vector<GLuint> ();
	std::vector<char> GeometryData_03::AllTexturesStr = {'N', 'o', 'n', 'e', '\0', '\0'};
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
		m_NumberOfGeometriesToRender = 1;

		OnAttachBase ();
		static bool first = true;
		if (first) {
			first = false;
			GeometryData_03::AddTextureOption ("assets/dice.png");
			/*{
				std::fstream file;
				file.open ("assets/noise.ppm", std::ios::out);

				uint16_t image_width = 600, image_height = 100;
				file << "P3\n" << image_width << ' ' << image_height << "\n255\n";

				for (int j = image_height-1; j >= 0; --j) {
					for (int i = 0; i < image_width; ++i) {
						auto r = double (i) / (image_width-1);
						auto g = double (j) / (image_height-1);
						auto b = 0.25;

						int ir = static_cast<int>(255.999 * r);
						int ig = static_cast<int>(255.999 * g);
						int ib = static_cast<int>(255.999 * b);

						file << ir << ' ' << ig << ' ' << ib << '\n';
					}
				}
				file.close ();

				GeometryData_03::AddTextureOption ("assets/noise.ppm");
			}*/
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
			if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Additional Material settings"))) {
				static GLuint display_texture = 0;
				static int selected = 0;
				ImGui::Text ("If you cannot see your uploaded texture here reselect from combo");
				ImGui::Image ((void *)(display_texture), ImVec2 (ImGui::GetContentRegionAvailWidth (), ImGui::GetContentRegionAvailWidth () / 4), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				if (ImGui::Combo ("already uploaded textures", &selected, GeometryData_03::AllTexturesStr.data ())) {
					display_texture = GeometryData_03::AllTextures[selected - 1];
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
						GeometryData_03::AddTextureOption (texture, textureName);
						break;
					case 1: // FBM
						textureName = "FBM2_" + std::to_string (texture);
						GeometryData_03::AddTextureOption (texture, textureName);
						break;
					case 2: // Turbulance
						textureName = "Turbulance_" + std::to_string (texture);
						GeometryData_03::AddTextureOption (texture, textureName);
						break;
					}
					display_texture = texture;
					selected = GeometryData_03::AllTextures.size ();
				}
				ImGui::Separator ();
				static int imprtFrmt = 0;
				if (ImGui::Button ("Add From Disk")) {
					display_texture = GeometryData_03::AddTextureOption (GLCore::Utils::FileDialogs::OpenFile ("Image\0*.jpeg\0*.png\0*.bmp\0*.hdr\0*.psd\0*.tga\0*.gif\0*.pic\0*.psd\0*.pgm\0").c_str (), Helper::TEXTURE_2D::MAPPING (imprtFrmt));
					selected = GeometryData_03::AllTextures.size ();
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