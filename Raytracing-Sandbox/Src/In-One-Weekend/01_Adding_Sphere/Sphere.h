#pragma once
#include <GLCore.h>
#include "../base.h"

namespace In_One_Weekend
{
	class Sphere: public ComputeAndSqrShader_Base
	{
	public:
		Sphere ()
			: ComputeAndSqrShader_Base ("InOneWeekend - 01_Sphere", "writing on Screen", Helper::ReadFileAsString ("./Src/In-One-Weekend/01_Adding_Sphere/computeShaderSrc.glsl", '#').c_str ())
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
		
		glm::vec3 FrontFromPitchYaw (float pitch, float yaw);
	private:
		bool m_ShowNormals = true;
		GLuint m_ComputeShaderOutputTex = 0;
	    GLuint m_FocusDistUniLoc = 0;
	    GLuint m_CamPosnUniLoc = 0;
	    GLuint m_CamDirnUniLoc = 0;
	    GLuint m_ShowNormalsUniLoc = 0;
	    GLuint m_SphereRadiusUniLoc = 0;
	    GLuint m_SpherePosnUniLoc = 0;
	    float m_FocusDist = 1.0f;
	    glm::vec3 m_CameraPosn = glm::vec3 (0, 1, 10);
	    glm::vec2 m_CameraPitchYaw = glm::vec2 (0, -90);
	    glm::vec4 m_Sphere = glm::vec4 (0, 3, -1, 3);

		const glm::ivec3 Work_Group_Count = { 0,0,0 }, Work_Group_Size = { 0,0,0 };
		glm::ivec2 m_OutputTexDimensions = glm::ivec2 (100, 100);
	};
};
