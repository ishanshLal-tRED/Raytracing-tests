#include "../base.h"

namespace In_Next_Week
{
	// 15 floats + 9 floats = 24(multiple of 4)
	struct Material
	{
		float  RefractiveIndex, Refractivity, Reflectivity;
		float  Scatteritivity[2];
		glm::vec3 Color;
	};
	struct GeometryBuff
		: public Transform_Buff // 18 float
	{
		float Type; // +1 float
		float _padding; // +1 float
		
		Material MTL; // +8 float = 28 = 7*4 floats or 112 bytes
	};
	enum class GeometryType: int
	{
		None = 0,
		Ellipsoid,
		Cuboid
	};
	class GeometryData
		: public Transform_Data
	{
	public:
		GeometryData (): Transform_Data () {};

		// virtual std::pair<glm::vec3, glm::vec3> CalculateBBMinMax () override
		
		virtual bool OnImGuiRender () override
		{
			return 
				  ImGui::SliderFloat2 ("Scatteritivity (OnRefract, OnReflect)", &Scatteritivity[0], 0.0f, 1.0f)
				| ImGui::SliderFloat ("Reflectivity", &Reflectivity, 0.0f, 1.0f - Refractivity)
				| ImGui::SliderFloat ("Refractivity", &Refractivity, 0.0f, 1.0f)
				| ImGui::SliderFloat ("Refractive Index", &RefractiveIndex, 1.0f, 3.0f)
				| ImGui::ColorEdit3 ("Color", &Color[0])
				| ImGui::Combo ("Geometry Type", (int *)&Type, "None\0Ellipsoid\0Cuboid\0")
				| OnImGuiRenderMin ();
		}

		void FillBuffer (GeometryBuff *buffer)
		{
			FillTransformBuff (buffer);
			buffer->Type = float (Type);
			buffer->MTL.Color = Color;

			buffer->MTL.RefractiveIndex = RefractiveIndex;
			buffer->MTL.Reflectivity = Reflectivity;
			buffer->MTL.Refractivity = Refractivity;

			buffer->MTL.Scatteritivity[0] = Scatteritivity[0], buffer->MTL.Scatteritivity[1] = Scatteritivity[1];
		}

		/*|
		|*| #define DEFAULT_COLOR            | vec3(0,0,0)
		|*| #define DEFAULT_REFRACTIVE_INDEX | 1.5
		|*| #define DEFAULT_REFRACTIVITY     | 0.7
		|*| #define DEFAULT_REFLECTIVITY     | 0.2
		|*| #define DEFAULT_SCATTER_REFRACT  | 0.0
		|*| #define DEFAULT_SCATTER_REFLECT  | 0.0
		|*/

		GeometryType Type = GeometryType::Ellipsoid;
		bool isDynamic = false;
		glm::vec3 Color = { 0,0,0 };
		float RefractiveIndex = 1.5f;
		float Refractivity = 0.65f;
		float Reflectivity = 0.15f;
		glm::vec2 Scatteritivity = { 0,0 }; // OnRefract, OnReflect
	};


	class BVH : public RT_Base<GeometryData, GeometryBuff> // private
	{
	public:
		BVH ()
			: RT_Base<GeometryData, GeometryBuff> ("In-Next-Week : BVH", "No discription", Helper::ReadFileAsString (".\\Src\\In-Next-Week\\01_BoundingVolumeHierarchy\\computeShaderSrc.glsl", '#').c_str (), s_FragShaderSrc)
		{}
		virtual ~BVH () = default;

		virtual void OnUpdate (GLCore::Timestep) override;
		virtual void OnAttach () override;
		virtual void OnDetach () override;
		virtual void OnEvent (GLCore::Event &event) override;
		virtual void OnImGuiRender () override;
		virtual void ImGuiMenuOptions () override {}

		virtual void OnComputeShaderReload () override;
		virtual void OnSquareShaderReload () override;

		virtual bool FillBuffer (GLCore::Timestep) override;

		static const char *s_FragShaderSrc;

		glm::vec3 m_DistBetweenSpheres = {1,1,1};
		GLuint m_BoolUseDepthTextureUniformID = 0;
		bool m_UseDepthTexture = false;
	};
}