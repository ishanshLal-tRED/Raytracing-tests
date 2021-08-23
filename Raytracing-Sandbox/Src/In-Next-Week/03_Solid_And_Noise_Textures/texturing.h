#include "../base.h"

namespace In_Next_Week
{
	// 15 floats + 9 floats = 24(multiple of 4)
	struct Material_03
	{
		float  RefractiveIndex;

		float Refractivity, Reflectivity;
		float  Scatteritivity[2];
		glm::vec3 Color;
		float TextureIndex; // NOTE: I'll cube-sphere cubic UV mapping for texturing instead of Mer-crate UV projection, as it can mapped to both cuboid and ellipsoid
	};
	struct GeometryBuff_03
		: public Transform_Buff // 18 float
	{
		float Type; // +1 float

		Material_03 MTL; // +9 float = 28 = 7*4 floats or 112 bytes
	};
	enum class GeometryType_03: int
	{
		None = 0,
		Ellipsoid,
		Cuboid
	};
	class GeometryData_03
		: public Transform_Data
	{
	public:
		GeometryData_03 (): Transform_Data () {};
		virtual ~GeometryData_03 () {
			if (AllTextures.size () > 0) {
				glDeleteTextures (AllTextures.size (), AllTextures.data ());
				AllTextures.clear ();
			}
		};

		// virtual std::pair<glm::vec3, glm::vec3> CalculateBBMinMax () override

		virtual bool OnImGuiRender () override
		{
			return
				  ImGui::Combo ("Texture", &TextureIndex, AllTexturesStr.c_str())
				| ImGui::SliderFloat2 ("Scatteritivity (OnRefract, OnReflect)", &Scatteritivity[0], 0.0f, 1.0f)
				| ImGui::SliderFloat ("Reflectivity", &Reflectivity, 0.0f, 1.0f - Refractivity)
				| ImGui::SliderFloat ("Refractivity", &Refractivity, 0.0f, 1.0f)
				| ImGui::SliderFloat ("Refractive Index", &RefractiveIndex, 1.0f, 3.0f)
				| ImGui::ColorEdit3 ("Color", &Color[0])
				| ImGui::Combo ("Geometry Type", (int *)&Type, "None\0Ellipsoid\0Cuboid\0")
				| OnImGuiRenderMin ();
		}

		void FillBuffer (GeometryBuff_03 *buffer)
		{
			FillTransformBuff (buffer);
			buffer->Type = float (Type);
			
			buffer->Type = float (Type);
			
			buffer->MTL.Color = Color;
			buffer->MTL.TextureIndex = float(TextureIndex);

			buffer->MTL.RefractiveIndex = RefractiveIndex;
			buffer->MTL.Reflectivity = Reflectivity;
			buffer->MTL.Refractivity = Refractivity;

			buffer->MTL.Scatteritivity[0] = Scatteritivity[0], buffer->MTL.Scatteritivity[1] = Scatteritivity[1];
		}

		// if Load From Disk Use GLCore::Utils::FileDialogs::OpenFile("Image\0*.jpeg\0*.png\0*.bmp\0*.hdr\0*.psd\0*.tga\0*.gif\0*.pic\0*.psd\0*.pgm\0").c_str () as input
		static void AddTextureOption (const char *filePath = nullptr)
		{
			auto temp = Helper::TEXTURE_2D::LoadFromDiskToGPU (filePath);
			if (temp) {
				auto [a, b, c] = temp.value ();
				AllTextures.push_back (a);
				uint32_t temp = AllTexturesStr.size ();
				AllTexturesStr += (' ' + std::filesystem::path (filePath).filename ().string ());
				AllTexturesStr[temp] = '\0';
			}
		}
		static void BindTextureOption ()
		{
			for (uint32_t i = 0; i < AllTextures.size (); i++) {
				glActiveTexture (GL_TEXTURE2 + i);
				glBindTexture (GL_TEXTURE_2D, AllTextures[i]);
			}
		}

		/*|
		|*| DEFAULT_COLOR            | vec3(0,0,0)
		|*| DEFAULT_REFRACTIVE_INDEX | 1.5
		|*| DEFAULT_REFRACTIVITY     | 0.7
		|*| DEFAULT_REFLECTIVITY     | 0.2
		|*| DEFAULT_SCATTER_REFRACT  | 0.0
		|*| DEFAULT_SCATTER_REFLECT  | 0.0
		|*/

		GeometryType_03 Type = GeometryType_03::Ellipsoid;
		bool isDynamic = false;
		glm::vec3 Color = { 0,0,0 };
		int TextureIndex = 0;
		float RefractiveIndex = 1.5f;
		float Refractivity = 0.65f;
		float Reflectivity = 0.15f;
		glm::vec2 Scatteritivity = { 0,0 }; // OnRefract, OnReflect

		static std::vector<GLuint> AllTextures;
		static std::string AllTexturesStr; // Split By '\0' to directly be used for ImGui::Combo
	};


	class Texturing: public RT_Base<GeometryData_03, GeometryBuff_03> // private
	{
	public:
		Texturing ()
			: RT_Base<GeometryData_03, GeometryBuff_03> ("In-Next-Week : Textures and Noise", "No discription", Helper::ReadFileAsString (".\\Src\\In-Next-Week\\03_Solid_And_Noise_Textures\\computeShaderSrc.glsl", '#').c_str ())
		{}
		virtual ~Texturing () = default;

		virtual void OnUpdate (GLCore::Timestep) override;
		virtual void OnAttach () override;
		virtual void OnDetach () override;
		virtual void OnEvent (GLCore::Event &event) override;
		virtual void OnImGuiRender () override;
		virtual void ImGuiMenuOptions () override {}

		virtual void OnComputeShaderReload () override;

		virtual bool FillBuffer (GLCore::Timestep) override;

	};
}