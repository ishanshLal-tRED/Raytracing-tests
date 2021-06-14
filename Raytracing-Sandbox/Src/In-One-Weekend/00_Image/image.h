#include <GLCore.h>
#include "../base.h"

namespace In_One_Weekend
{
	class Image: public ComputeAndSqrShader_Base
	{
	public:
		Image ()
			: ComputeAndSqrShader_Base ("InOneWeekend - 01_Image", "writing on Screen")
		{}
		~Image () = default;
		virtual void OnDetach () override;
		virtual void OnAttach () override;
		virtual void OnUpdate (GLCore::Timestep ts);
		virtual void OnImGuiRender () override;
		virtual void ImGuiMenuOptions () override {};
	private:
		GLuint m_ComputeShaderOutputTex = 0;
		const glm::ivec3 Work_Group_Count = { 0,0,0 }, Work_Group_Size = { 0,0,0 };
		const glm::ivec2 m_OutputTexDimensions = glm::ivec2 (100, 100);
	};
};