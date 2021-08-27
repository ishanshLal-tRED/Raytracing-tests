#include "../base.h"

namespace In_Next_Week
{
	// 15 floats + 9 floats = 24(multiple of 4)
	struct Material_04
	{
		float  RefractiveIndex;

		float Refractivity, Reflectivity;
		float  Scatteritivity[2];
		glm::vec3 Color;
		float TextureIndex; // NOTE: I'll cube-sphere cubic UV mapping for texturing instead of Mer-crate UV projection, as it can mapped to both cuboid and ellipsoid
	};
	struct GeometryBuff_04
		: public Transform_Buff // 18 float
	{
		float Type; // +1 float

		Material_04 MTL; // +9 float = 28 = 7*4 floats or 112 bytes
	};
	enum class GeometryType_04: int
	{
		None = 0,
		Ellipsoid,
		Cuboid
	};
	class GeometryData_04
		: public Transform_Data
	{
	public:
		GeometryData_04 (): Transform_Data () {};
		virtual ~GeometryData_04 () {
			for (auto &iVal : AllTextures) {
				glDeleteTextures (1, &iVal.first);
			}
			AllTextures.clear ();
		};

		virtual std::pair<glm::vec3, glm::vec3> CalculateBBMinMax ()
		{
			glm::mat3 matrix =
				Helper::MATH::MakeRotationZ (glm::radians (rotation.z)) // roll   <-can be pitch roll respectivly but who cares
				*Helper::MATH::MakeRotationX (glm::radians (rotation.x)) // pitch <-'
				*Helper::MATH::MakeRotationY (glm::radians (rotation.y)); // yaw
			if (Type == GeometryType_04::Ellipsoid) {
				matrix = matrix*glm::mat3 (
					scale.x, 0, 0,
					0, scale.y, 0,
					0, 0, scale.z
				);
				float x = sqrt (matrix[0][0]*matrix[0][0] + matrix[1][0]*matrix[1][0] + matrix[2][0]*matrix[2][0]);
				float y = sqrt (matrix[0][1]*matrix[0][1] + matrix[1][1]*matrix[1][1] + matrix[2][1]*matrix[2][1]);
				float z = sqrt (matrix[0][2]*matrix[0][2] + matrix[1][2]*matrix[1][2] + matrix[2][2]*matrix[2][2]);

				glm::vec3 min (MIN (position.x, last_position.x), MIN (position.y, last_position.y), MIN (position.z, last_position.z));
				glm::vec3 max (MAX (position.x, last_position.x), MAX (position.y, last_position.y), MAX (position.z, last_position.z));
				return { glm::vec3 (-x, -y, -z) + min, glm::vec3 (x, y, z) + max };
			} 
			else if (Type == GeometryType_04::Cuboid)
			{
				glm::vec3 BBmin(0.0f), BBmax (0.0f);
				for (uint8_t i = 0; i < 8; i++)
				{
					glm::vec3 coord;
					uint8_t axis_bit = 1; // x axis
					for (uint8_t axis = 0; axis < 3; axis++)
					{
						coord[axis] = (i & axis_bit) ? 0.5f*scale[axis] : -0.5f*scale[axis];
						axis_bit = axis_bit << 1;
					}
					coord = matrix*coord;
					for (uint8_t axis = 0; axis < 3; axis++) {
						BBmin[axis] = MIN(coord[axis], BBmin[axis]);
						BBmax[axis] = MAX(coord[axis], BBmax[axis]);
					}
				}

				glm::vec3 min (MIN (position.x, last_position.x), MIN (position.y, last_position.y), MIN (position.z, last_position.z));
				glm::vec3 max (MAX (position.x, last_position.x), MAX (position.y, last_position.y), MAX (position.z, last_position.z));
				return { BBmin + min, BBmax + max };
			} else LOG_ASSERT (false);
			return { glm::vec3 (0), glm::vec3 (0) };
		}

		virtual bool OnImGuiRender () override
		{
			bool Changed = 
				ImGui::Checkbox ("Is Light Source", &isEmissive)
				| ImGui::SliderFloat2 ("Scatteritivity (OnRefract, OnReflect)", &Scatteritivity[0], 0.0f, 1.0f)
				| ImGui::ColorEdit3 ("Color", &Color[0])
				| ImGui::Combo ("Geometry Type", (int *)&Type, "None\0Ellipsoid\0Cuboid\0")
				| OnImGuiRenderMin ();
			if (!isEmissive) {
				Changed |=
					  ImGui::SliderFloat ("Reflectivity", &Reflectivity, 0.0f, 1.0f - Refractivity)
					| ImGui::SliderFloat ("Refractivity", &Refractivity, 0.0f, 1.0f)
					| ImGui::SliderFloat ("Refractive Index", &RefractiveIndex, 1.0f, 3.0f);
			}
			auto prev = TextureIndex;
			if (ImGui::Combo ("Texture", &TextureIndex, AllTexturesStr.data ())) {
				Changed = true;
				// decrease counter
				if (prev) {
					LOG_ASSERT (AllTextures[prev - 1].second > 0);
					AllTextures[prev - 1].second--;
				}
				if (TextureIndex) {
					AllTextures[TextureIndex - 1].second++;
				}
			}
			return Changed;
		}

		void FillBuffer (GeometryBuff_04 *buffer)
		{
			FillTransformBuff (buffer);
			buffer->Type = float (Type);
			
			buffer->Type = float (Type);
			
			uint32_t counter = 0;
			for (uint32_t i = 0; i < AllTextures.size (); i++) {
				if (AllTextures[i].second > 0) counter++;
				if (TextureIndex == i) break;
			}
			buffer->MTL.TextureIndex = float(counter);

			if (!isEmissive) {
				buffer->MTL.Color = Color;
				buffer->MTL.RefractiveIndex = RefractiveIndex;
				buffer->MTL.Reflectivity = Reflectivity;
				buffer->MTL.Refractivity = Refractivity;
			} else {
				buffer->MTL.Color = glm::vec3(1);
				buffer->MTL.RefractiveIndex = 1.0f;
				buffer->MTL.Reflectivity = 0.0f;
				buffer->MTL.Refractivity = 0.0f;
			}
			buffer->MTL.Scatteritivity[0] = Scatteritivity[0], buffer->MTL.Scatteritivity[1] = Scatteritivity[1];
		}

		// if Load From Disk Use GLCore::Utils::FileDialogs::OpenFile("Image\0*.jpeg\0*.png\0*.bmp\0*.hdr\0*.psd\0*.tga\0*.gif\0*.pic\0*.psd\0*.pgm\0").c_str () as input
		static GLuint AddTextureOption (const char *filePath = nullptr, Helper::TEXTURE_2D::MAPPING loadAs = Helper::TEXTURE_2D::MAPPING::CUBIC)
		{
			std::optional<std::tuple<GLuint, uint32_t, uint32_t>> temp;
			if (loadAs != Helper::TEXTURE_2D::MAPPING::CUBIC)
				temp = Helper::TEXTURE_2D::LoadFromDiskToGPU (filePath, loadAs, Helper::TEXTURE_2D::MAPPING::CUBIC);
			else temp = Helper::TEXTURE_2D::LoadFromDiskToGPU (filePath);
			if (temp) {
				auto [a, b, c] = temp.value ();
				AllTextures.push_back ({ a, 0 });
				std::string name = std::filesystem::path (filePath).filename ().string ();
				AllTexturesStr.insert(AllTexturesStr.end() - 1, '\0');
				for (auto r_itr = name.begin (); r_itr != name.end (); r_itr++) {
					AllTexturesStr.insert (AllTexturesStr.end () - 2, *r_itr);
				}
				return a;
			}
			return 0;
		}
		static void AddTextureOption (GLuint textureID, const std::string &name)
		{
			LOG_ASSERT(textureID != 0 && name != std::string());
			{
				AllTextures.push_back ({ textureID, 0 });
				AllTexturesStr.insert (AllTexturesStr.end () - 1, '\0');
				for (auto r_itr = name.begin (); r_itr != name.end (); r_itr++) {
					AllTexturesStr.insert (AllTexturesStr.end () - 2, *r_itr);
				}
			}
		}
		static void BindExtraData ()
		{
			std::vector<GLuint> send;
			for (auto& iVal : AllTextures)
				if (iVal.second > 0) send.push_back (iVal.first);

			uint32_t Size = MIN (send.size (), MaxTextureBindSlots);
			for (uint32_t i = 0; i < Size; i++) {
				glActiveTexture (GL_TEXTURE2 + i);
				glBindTexture (GL_TEXTURE_2D, send[i]);
			}

		
		}
		struct LightClass
		{
			float BBmin[3];
			float BBmax[3];
			GLuint idx;
		};
		
		GeometryType_04 Type = GeometryType_04::Ellipsoid;
		bool isEmissive = false;
		glm::vec3 Color = { 0,0,0 };
		int TextureIndex = 0;
		float RefractiveIndex = 1.5f;
		float Refractivity = 0.65f;
		float Reflectivity = 0.15f;
		glm::vec2 Scatteritivity = { 0,0 }; // OnRefract, OnReflect

		// TextureID, counter
		static uint32_t MaxTextureBindSlots, OffsetBindSlots;
		static std::vector<std::pair<GLuint, uint32_t>> AllTextures;
		static std::vector<char> AllTexturesStr; // Split By '\0' to directly be used for ImGui::Combo
	};


	class Lights: public RT_Base<GeometryData_04, GeometryBuff_04> // private
	{
	public:
		Lights ()
			: RT_Base<GeometryData_04, GeometryBuff_04> ("In-Next-Week : Lights, Camera And Action", "No discription", Helper::ReadFileAsString (".\\Src\\In-Next-Week\\04_Lights_Camera_And_Action\\computeShaderSrc.glsl", '#').c_str ())
		{}
		virtual ~Lights () = default;

		virtual void OnUpdate (GLCore::Timestep) override;
		virtual void OnAttach () override;
		virtual void OnDetach () override;
		virtual void OnEvent (GLCore::Event &event) override;
		virtual void OnImGuiRender () override;
		virtual void ImGuiMenuOptions () override {}

		virtual void OnComputeShaderReload () override;

		virtual bool FillBuffer (GLCore::Timestep) override;

		static constexpr uint32_t SSBO_LightIdentifiers = 0;

		GLuint m_ComputeShaderInput_SSBOLights = 0;
		GLuint m_UniformIDNumOfLightSources = 0;
		std::vector<GeometryData_04::LightClass> m_LightSSBOBuff;

		uint32_t LastIterationSSBOSize = 0;
		int m_MaxTextureBindSlots = 6;
	};
}