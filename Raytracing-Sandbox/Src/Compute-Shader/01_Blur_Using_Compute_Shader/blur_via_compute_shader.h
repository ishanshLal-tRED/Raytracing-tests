#pragma once
#include "../00_Basic_Compute_Shader/basic_compute_shader.h"

class BlurWithComputeShader_Test
	: public BasicComputeShader_Test
{
public:
	BlurWithComputeShader_Test (const char* name = "Blur using compute shader");
	virtual void OnAttachExtras ();
	virtual void OnDetachExtras ();
	virtual void OnImGuiRenderUnderSettingsTab ();
	virtual void OnUpdate (GLCore::Timestep ts) override;
private:
	struct
	{
		GLuint ID = 0;
		uint32_t width = 0;
		uint32_t height = 0;
	} m_ImageToBeBlurred;

	GLuint m_ComputeShaderOutputTex2 = 0;
	GLuint m_U_area_of_influence = 0;
	int m_area_of_influence = 1;
	int m_NumOfBlurIterations = 5;
	static const char *s_default_blur_compute_shader;
};