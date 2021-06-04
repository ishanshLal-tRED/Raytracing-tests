#include "basic_compute_shader.h"
#include "Utilities/utility.h"

using namespace GLCore;
using namespace GLCore::Utils;


const char *BasicComputeShader_Test::s_default_compute_shader = R"(
#version 440 core
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

vec4 out_Pixel(ivec2 pixel_coords){
	ivec2 imageSize = imageSize(img_output);
	float r = pixel_coords.x / (imageSize.x - 1.0);
    float g = pixel_coords.y / (imageSize.y - 1.0);
    float b = 0.25;
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
const char *BasicComputeShader_Test::s_default_sqr_shader_vert = R"(
#version 440 core
layout (location = 0) in vec3 in_Position;

layout (location = 0) out vec2 b_Coord;

void main()
{
	gl_Position = vec4(in_Position, 1.0f);
	b_Coord = vec2(in_Position.x > 0 ? 1.0 : 0, in_Position.y > 0 ? 1.0 : 0);
})";
const char *BasicComputeShader_Test::s_default_sqr_shader_frag = R"(
#version 440 core
layout (location = 0) in vec2 b_Coord;
layout (rgba32f, binding = 0) uniform sampler2D u_Texture;

layout (location = 0) out vec4 o_Color;

void main()
{
	o_Color = texture(u_Texture, b_Coord);
})";
int Work_Group_Count[3] = { 0,0,0 }, Work_Group_Size[3] = { 0,0,0 };

BasicComputeShader_Test::BasicComputeShader_Test(const char* name /*= "Basic Compute Shader"*/, const char* discription /*= "creating a basic compute shader which sets each pixel and displays it on-to screen acc to Square_Shader" */
	, const char* default_compute_shader_src /*= s_default_compute_shader */, const char* default_sqr_shader_vert_src /*= s_default_sqr_shader_vert */, const char* default_sqr_shader_frag_src /*= s_default_sqr_shader_frag */
) : TestBase (std::string(name), std::string(discription)),
m_ComputeShaderTXT (Buffer::Create (default_compute_shader_src)),
m_SquareShaderTXT_vert (Buffer::Create (default_sqr_shader_vert_src, 512)),
m_SquareShaderTXT_frag (Buffer::Create (default_sqr_shader_frag_src, 512))
{}

void BasicComputeShader_Test::ReloadComputeShader (){
	std::optional<GLuint> shader_program = Helper::SHADER::CreateProgram (m_ComputeShaderTXT.data (), GL_COMPUTE_SHADER);
	if (shader_program.has_value ()) {
		if(m_ComputeShaderProgID)
			glDeleteProgram (m_ComputeShaderProgID);
		{// check whether last bound program is this, if yes then update it
			GLuint last_program;
			glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *)&last_program);
			if (m_ComputeShaderProgID == last_program)
				glUseProgram (shader_program.value ());
		}
		m_ComputeShaderProgID = shader_program.value ();
	}
}
void BasicComputeShader_Test::ReloadSquareShader  (){
	std::optional<GLuint> shader_program = Helper::SHADER::CreateProgram (m_SquareShaderTXT_vert.data (), GL_VERTEX_SHADER, m_SquareShaderTXT_frag.data (), GL_FRAGMENT_SHADER);
	if (shader_program.has_value ()) {
		if (m_SquareShaderProgID)
			glDeleteProgram (m_SquareShaderProgID);
		{// check whether last bound program is this, if yes then update it
			GLuint last_program;
			glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *)&last_program);
			if (m_SquareShaderProgID == last_program)
				glUseProgram (shader_program.value ());
		}
		m_SquareShaderProgID = shader_program.value ();
	}
}
void BasicComputeShader_Test::OnAttach ()
{
	EnableGLDebugging ();

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ReloadComputeShader (); ReloadSquareShader ();
	{
		glGenVertexArrays (1, &m_QuadVA);
		glBindVertexArray (m_QuadVA);

		float vertices[] = {
			-0.8f, -0.8f, 0.0f,
			 0.8f, -0.8f, 0.0f,
			 0.8f,  0.8f, 0.0f,
			-0.8f,  0.8f, 0.0f
		};

		glGenBuffers (1, &m_QuadVB);
		glBindBuffer (GL_ARRAY_BUFFER, m_QuadVB);
		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof (float) * 3, 0);

		uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
		glGenBuffers (1, &m_QuadIB);
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_QuadIB);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);
	}
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
	if (!m_ComputeShaderOutputTex){
		// dimensions of the image
		int tex_w = m_OutputTexDimensions.x, tex_h = m_OutputTexDimensions.y;
		GLuint &tex_output = m_ComputeShaderOutputTex;
		glGenTextures (1, &tex_output);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture   (GL_TEXTURE_2D, tex_output);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);

		// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
		glBindImageTexture (0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}
	OnAttachExtras ();
}
void BasicComputeShader_Test::OnDetach ()
{
	glDeleteVertexArrays (1, &m_QuadVA);
	m_QuadVA = 0;
	glDeleteBuffers (1, &m_QuadVB);
	m_QuadVB = 0;
	glDeleteBuffers (1, &m_QuadIB);
	m_QuadIB = 0;

	glDeleteTextures (1, &m_ComputeShaderOutputTex);
	m_ComputeShaderOutputTex = 0;

	glDeleteProgram (m_ComputeShaderProgID);
	m_ComputeShaderProgID = 0;
	glDeleteProgram (m_SquareShaderProgID);
	m_SquareShaderProgID = 0;

	OnDetachExtras ();
}

void BasicComputeShader_Test::OnUpdate (GLCore::Timestep ts)
{
	glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	{ // launch compute shaders!
		glUseProgram (m_ComputeShaderProgID);
		glDispatchCompute (m_OutputTexDimensions.x, m_OutputTexDimensions.y, 1);
	}

	// make sure writing to image has finished before read
	glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	{ // normal drawing pass
		glClear (GL_COLOR_BUFFER_BIT);
		glUseProgram (m_SquareShaderProgID);
		glBindVertexArray (m_QuadVA);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputTex);
		glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}
}
void BasicComputeShader_Test::OnImGuiRender ()
{
	ImGui::Begin (ImGuiLayer::UniqueName("Just a window"));
	if (ImGui::BeginTabBar (ImGuiLayer::UniqueName("Shaders Content"))) {
		if (ImGui::BeginTabItem(ImGuiLayer::UniqueName("Settings"))) {
			OnImGuiRenderUnderSettingsTab ();
			ImGui::Text ("Max Compute Work Group\n Count: %d, %d, %d\n Size:  %d, %d, %d", Work_Group_Count[0], Work_Group_Count[1], Work_Group_Count[2], Work_Group_Size[0], Work_Group_Size[1], Work_Group_Size[2]);
			ImGui::EndTabItem ();
		}
		if (ImGui::BeginTabItem(ImGuiLayer::UniqueName("Compute Shader Source"))) {
			ImVec2 contentRegion = ImGui::GetContentRegionAvail ();
			ImVec2 textbox_contentRegion(contentRegion.x, contentRegion.y - 60);
			ImVec2 button_contentRegion(contentRegion.x, 50);

			ImGui::InputTextMultiline ("Compute Shader Source: ", m_ComputeShaderTXT.raw_data (), m_ComputeShaderTXT.size (), textbox_contentRegion);
			if (ImGui::Button ("Reload Compute Shader", button_contentRegion)) {
				ReloadComputeShader ();
			}
			ImGui::EndTabItem ();
		}
		if (ImGui::BeginTabItem(ImGuiLayer::UniqueName("Square Shader Source"))) {
			ImVec2 contentRegion = ImGui::GetContentRegionAvail ();
			contentRegion.y /= 2.0f;
			ImVec2 textbox_contentRegion (contentRegion.x, contentRegion.y - 60);
			ImVec2 button_contentRegion (contentRegion.x, 60);

			ImGui::Text ("Vertex SRC:");
			ImGui::InputTextMultiline ("Vert SRC", m_SquareShaderTXT_vert.raw_data (), m_SquareShaderTXT_vert.size (), textbox_contentRegion);
			
			ImGui::Separator ();
			
			ImGui::Text ("Fragment SRC:");
			ImGui::InputTextMultiline ("Frag SRC", m_SquareShaderTXT_frag.raw_data (), m_SquareShaderTXT_frag.size (), textbox_contentRegion);
			
			if (ImGui::Button ("Reload Square Shader", button_contentRegion)) {
				ReloadSquareShader ();
			}
			
			ImGui::EndTabItem ();
		}
		ImGui::EndTabBar ();
	}
	ImGui::End ();
}
void BasicComputeShader_Test::ImGuiMenuOptions ()
{
}