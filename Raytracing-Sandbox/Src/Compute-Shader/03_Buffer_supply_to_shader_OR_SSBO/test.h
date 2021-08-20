#pragma once
#include <GLCore.h>
#include <GLCoreUtils.h>
#include "In-One-Weekend/base.h"

namespace BuffSupply
{
	class Test: public ComputeAndSqrShader_Base
	{
	public:
		Test ()
			: ComputeAndSqrShader_Base ("Test Shader Buffers", "Just clear color on Screen", s_ComputeShaderSRC)
		{}
		virtual ~Test () = default;
		virtual void OnDetach () override;
		virtual void OnAttach () override;
		virtual void OnUpdate (GLCore::Timestep ts);
		virtual void OnImGuiRender () override;
		virtual void ImGuiMenuOptions () override {};

		virtual void OnComputeShaderReload () override;
	private:
		bool m_ReDraw = true;
		GLuint m_ComputeShaderOutput_Tex = 0;
		GLuint m_ComputeShaderInput_SSBOColr = 0; // float[3]
		
		const glm::ivec2 m_OutputTexDimensions = glm::ivec2 (100, 100);
		
		const glm::ivec3 Work_Group_Count = { 0,0,0 }, Work_Group_Size = { 0,0,0 };

		glm::vec3 m_Color = glm::vec3(0.5f);
	private:
		static const char *s_ComputeShaderSRC;
	};
}