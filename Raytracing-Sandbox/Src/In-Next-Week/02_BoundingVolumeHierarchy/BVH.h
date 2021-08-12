#include "../base2.h"

// 15 floats + 9 floats = 24(multiple of 4)
struct GeometryBuff
	: public Transform_Buff
{
	float Type;
	float Color[3];
	float RefractiveIndex;
	float Refractivity;
	float Reflectivity;
	float Scatteritivity[2];
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

	virtual std::pair<glm::vec3, glm::vec3> CalculateBBMinMax () override
	{
		return { glm::vec3 (0), glm::vec3 (0) };
	}
	virtual void OnImGuiRender () override
	{
		OnImGuiRenderMin ();
		ImGui::ColorEdit3 ("Color", &Color[0]);
		ImGui::SliderFloat ("Refractive Index", &RefractiveIndex, 1.0f, 3.0f);
		ImGui::SliderFloat ("Refractivity", &Refractivity, 0.0f, 1.0f);
		ImGui::SliderFloat ("Reflectivity", &Reflectivity, 0.0f, 1.0f - Refractivity);
		ImGui::SliderFloat2 ("Scatteritivity (OnRefract, OnReflect)", &Scatteritivity[0], 0.0f, 1.0f);
	}

	void FillBuffer (GeometryBuff *buffer)
	{
		FillTransformBuff (buffer);
		buffer->Type = float (Type);
		buffer->Color[0] = Color[0], buffer->Color[1] = Color[1], buffer->Color[2] = Color[2];

		buffer->RefractiveIndex = RefractiveIndex;
		buffer->Reflectivity = Reflectivity;
		buffer->Refractivity = Refractivity;

		buffer->Scatteritivity[0] = Scatteritivity[0], buffer->Scatteritivity[1] = Scatteritivity[1];
	}

	GeometryType Type = GeometryType::Ellipsoid;
	glm::vec3 Color = { 1,0,0 };
	float RefractiveIndex = 1.0f;
	float Refractivity = 0;
	float Reflectivity = 0;
	glm::vec2 Scatteritivity = { 0,0 }; // OnRefract, OnReflect
};
class BVH
	: public RayTracing_Base<GeometryData, GeometryBuff>
{
public:
	BVH()
		: RayTracing_Base<GeometryData, GeometryBuff> ("InNextWeek - 01_BoundingVolumeHierarchy", "Fast RT with LBVH", Helper::ReadFileAsString (".\\Src\\In-Next-Week\\02_BoundingVolumeHierarchy\\new_ComputeShaderSrc.glsl", '#').c_str ())
	{}
	virtual ~BVH () = default;
	virtual void OnDetach () override;
	virtual void OnAttach () override;
	virtual void OnUpdate (GLCore::Timestep ts);
	virtual void OnEvent (GLCore::Event &event) override;
	virtual void OnImGuiRender () override;
	virtual void ImGuiMenuOptions () override {};

	virtual void OnComputeShaderReload () override;
	virtual void OnSquareShaderReload () override;

	virtual bool FillBuffer () override;
};