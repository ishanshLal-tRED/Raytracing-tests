#pragma once
#include <GLCore.h>
#include "../base.h"

namespace In_One_Weekend
{
	class Sphere: public ComputeAndSqrShader_Base
	{
	public:
		Sphere ()
			: ComputeAndSqrShader_Base ("InOneWeekend - 01_Sphere", "writing on Screen", s_ComputeShader)
		{}
		~Sphere () = default;
		virtual void OnDetach () override;
		virtual void OnAttach () override;
		virtual void OnUpdate (GLCore::Timestep ts);
		virtual void OnEvent (GLCore::Event& event) override;
		virtual void OnImGuiRender () override;
		virtual void ImGuiMenuOptions () override {};

		virtual void OnComputeShaderReload () override;
		virtual void OnSquareShaderReload  () override;
	private:
		GLuint m_ComputeShaderOutputTex = 0;
		const glm::ivec3 Work_Group_Count = { 0,0,0 }, Work_Group_Size = { 0,0,0 };
		glm::ivec2 m_OutputTexDimensions = glm::ivec2 (100, 100);
	    GLuint m_FocusDistUniLoc = 0;
	    GLuint m_CamPosnUniLoc = 0;
	    GLuint m_CamDirnUniLoc = 0;
	    float m_FocusDist = 1.0f;
	    glm::vec3 m_CameraPosn = glm::vec3 (0);
	    glm::vec3 m_CameraDirn = glm::vec3 (0, 0, -1);

		static const char *s_ComputeShader;
	};
};
