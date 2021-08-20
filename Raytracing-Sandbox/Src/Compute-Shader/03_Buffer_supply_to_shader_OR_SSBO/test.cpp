#include "test.h"

namespace BuffSupply
{
	const char *Test::s_ComputeShaderSRC = R"(
#version 440 core

layout (local_size_x = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

// No Binding
layout(std430) buffer sbo_Color{
	float colr[];
};

vec4 out_Pixel(ivec2 pixel_coords){
	float r = colr[0];
    float g = colr[1];
    float b = colr[2];
	return vec4(r, g, b, 1.0);
}
void main() {
  // get index in global work group i.e x,y position
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
  
  // base pixel colour for image
  vec4 pixel = out_Pixel(pixel_coords);
  
  // output to a specific pixel in the image
  imageStore(img_output, pixel_coords, pixel);
})";

	enum class SSBO_Index : uint32_t
	{
		COLOR = 1,
		Total

	};

	void Test::OnAttach ()
	{
		GLCore::Utils::EnableGLDebugging ();

		glEnable (GL_DEPTH_TEST);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		ReloadComputeShader (); ReloadSquareShader ();
		ReGenQuadVAO ();
		{
			int *work_grp_cnt = (int *)((void *)(&Work_Group_Count[0]));
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

			int *work_grp_size = (int *)((void *)(&Work_Group_Size[0]));
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
		}
		if (!m_ComputeShaderOutput_Tex) {
			// dimensions of the image
			int tex_w = m_OutputTexDimensions.x, tex_h = m_OutputTexDimensions.y;

			m_ComputeShaderOutput_Tex = Helper::TEXTURE_2D::Upload (nullptr, tex_w, tex_h, GL_RGBA32F, GL_RGBA, GL_FLOAT);

			// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
			glBindImageTexture (0, m_ComputeShaderOutput_Tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		}

		{
			GLint val;
			glGetIntegerv (GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &val);
			LOG_ASSERT (uint32_t (SSBO_Index::Total) <= uint32_t(val));
		}

		if (!m_ComputeShaderInput_SSBOColr) {
			glGenBuffers (1, &m_ComputeShaderInput_SSBOColr);
			glBindBuffer (GL_SHADER_STORAGE_BUFFER, m_ComputeShaderInput_SSBOColr);
			glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (m_Color), &m_Color[0], GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
			glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0); // unbind
		}
	}
	void Test::OnDetach ()
	{
		DeleteQuadVAO ();

		glDeleteTextures (1, &m_ComputeShaderOutput_Tex);
		m_ComputeShaderOutput_Tex = 0;

		glDeleteBuffers (1, &m_ComputeShaderInput_SSBOColr);
		m_ComputeShaderInput_SSBOColr = 0;

		DeleteComputeShader ();
		DeleteSquareShader ();
	}
	void Test::OnUpdate (GLCore::Timestep ts)
	{
		glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		if (m_ReDraw) { // launch compute shader!
			m_ReDraw = false;
			glUseProgram (m_ComputeShaderProgID);

			{
				glBindBuffer (GL_SHADER_STORAGE_BUFFER, m_ComputeShaderInput_SSBOColr);
				glBindBufferBase (GL_SHADER_STORAGE_BUFFER, uint32_t (SSBO_Index::COLOR), m_ComputeShaderInput_SSBOColr);
				glBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, sizeof (m_Color), &m_Color[0]);
			}

			glDispatchCompute (m_OutputTexDimensions.x, m_OutputTexDimensions.y, 1);
			// make sure writing to image has finished before read
			glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glClear (GL_COLOR_BUFFER_BIT);
		}


		{ // normal drawing pass
			glUseProgram (m_SquareShaderProgID);
			glBindVertexArray (m_QuadVA);
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutput_Tex);
			glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		}
	}
	void Test::OnImGuiRender ()
	{
		using namespace GLCore;
		ImGui::Begin (ImGuiLayer::UniqueName ("Just a window"));
		if (ImGui::BeginTabBar (ImGuiLayer::UniqueName ("Shaders Content"))) {
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Settings"))) {

				ImGui::Text ("Max Compute Work Group\n Count: %d, %d, %d\n Size:  %d, %d, %d", Work_Group_Count[0], Work_Group_Count[1], Work_Group_Count[2], Work_Group_Size[0], Work_Group_Size[1], Work_Group_Size[2]);

				m_ReDraw |= ImGui::ColorEdit3 ("Clear Color", &m_Color[0]);

				ImGui::EndTabItem ();
			}
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Compute Shader Source"))) {

				OnImGuiComputeShaderSource ();

				ImGui::EndTabItem ();
			}
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Square Shader Source"))) {

				OnImGuiSqureShaderSource ();

				ImGui::EndTabItem ();
			}
			ImGui::EndTabBar ();
		}
		ImGui::End ();
	}
	
	void Test::OnComputeShaderReload ()
	{
		GLuint blockIndex = glGetProgramResourceIndex (m_ComputeShaderProgID, GL_SHADER_STORAGE_BLOCK, "sbo_Color");
		glShaderStorageBlockBinding (m_ComputeShaderProgID, blockIndex, uint32_t (SSBO_Index::COLOR));
	}
}