#include "blur_via_compute_shader.h"
#include "Utilities/utility.h"

using namespace GLCore;

const char *BlurWithComputeShader_Test::s_default_blur_compute_shader = R"(
#version 440 core
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

uniform int u_AreaOfInfluence;
uniform sampler2D u_Texture;

vec4 out_Pixel(ivec2 pixel_coords, image2D img_output)
{
  ivec2 imageSize = imageSize(img_output);
  ivec2 texSize = textureSize(u_Texture, 0);
  float _x = (float(pixel_coords.x)/imageSize.x)*texSize.x, _y = (float(pixel_coords.y)/imageSize.y)*texSize.y; 
  float r = 0.0f, g = 0.0f, b = 0.0f;
  for(int x = -u_AreaOfInfluence; x < u_AreaOfInfluence+1; x++){
	for(int y = -u_AreaOfInfluence; y < u_AreaOfInfluence+1; y++){
      vec2 nrml_coord = vec2((_x+x)/(texSize.x - 1.0),(_y+y)/(texSize.y - 1.0));
      vec4 colr = texture(u_Texture, nrml_coord);
      r+=colr.x, g+=colr.y, b+=colr.z;
  }}
  float times = u_AreaOfInfluence*2.0f + 1;
  times*=times;
  return vec4(r/times, g/times, b/times, 1.0);
}
void main() {
  // get index in global work group i.e x,y position
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
  
  // base pixel colour for image
  vec4 pixel = out_Pixel(pixel_coords, img_output);
  
  // output to a specific pixel in the image
  imageStore(img_output, pixel_coords, pixel);
})";
BlurWithComputeShader_Test::BlurWithComputeShader_Test(const char* name /*= "Blur using compute shader"*/) 
	: BasicComputeShader_Test (name, "blurring texture at runtime then displaying it onto screen"
							   , s_default_blur_compute_shader
							   //,BasicComputeShader_Test::s_default_sqr_shader_vert
							   //,BasicComputeShader_Test::s_default_sqr_shader_frag
							   )
{}

void BlurWithComputeShader_Test::OnAttachExtras ()
{
	if(!m_ImageToBeBlurred.ID){
		auto return_val = Helper::TEXTURE_2D::LoadFromDiskToGPU ("assets/trex.png");
		if (return_val.has_value ())
		{
			auto [texID, wid, ht] = return_val.value ();
			m_ImageToBeBlurred.ID = texID, m_ImageToBeBlurred.width = wid, m_ImageToBeBlurred.height = ht;
		}
	}
	if (!m_ComputeShaderOutputTex2) {
		// dimensions of the image
		int tex_w = m_OutputTexDimensions.x, tex_h = m_OutputTexDimensions.y;
		GLuint &tex_output = m_ComputeShaderOutputTex2;
		glGenTextures (1, &tex_output);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, tex_output);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);

		// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
		//glBindImageTexture (0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}
	m_U_area_of_influence = glGetUniformLocation (m_ComputeShaderProgID, "u_AreaOfInfluence");
}
void BlurWithComputeShader_Test::OnDetachExtras ()
{
	glDeleteTextures (1, &m_ComputeShaderOutputTex2);
	m_ComputeShaderOutputTex2 = 0;

	m_U_area_of_influence = 0;
	
	// not deleting m_ImageTobeblurred
}
static uint32_t iteratePosn = 0;
static uint32_t reCheckiteratePosn = 0;
void BlurWithComputeShader_Test::OnImGuiRenderUnderSettingsTab ()
{
	ImGui::Text ("Num Of Actual Iterations: %d, recheck: %d", iteratePosn, reCheckiteratePosn);
	ImGui::DragInt ("No. Of Blur Iterations", &m_NumOfBlurIterations, 1);
	ImGui::DragInt ("Area Of Influence (can massively slow down system, Don't know why):", &m_area_of_influence, 0.2f, 0, 10);
}
void BlurWithComputeShader_Test::OnUpdate (GLCore::Timestep ts)
{
	glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram (m_ComputeShaderProgID);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, m_ImageToBeBlurred.ID);
	reCheckiteratePosn = 0;
	for (iteratePosn = 0; iteratePosn < m_NumOfBlurIterations; iteratePosn++)
	{
		reCheckiteratePosn++;
		glUniform1i (m_U_area_of_influence, m_area_of_influence);
		if (iteratePosn != 0){
			glBindImageTexture (0, iteratePosn % 2 ? m_ComputeShaderOutputTex2 : m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glBindTexture (GL_TEXTURE_2D, iteratePosn % 2 ? m_ComputeShaderOutputTex : m_ComputeShaderOutputTex2);
		} else {
			glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glBindTexture (GL_TEXTURE_2D, m_ImageToBeBlurred.ID);
		}

		glDispatchCompute (m_OutputTexDimensions.x, m_OutputTexDimensions.y, 1);
		// make sure writing to image has finished before read
		glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}


	{ // normal drawing pass
		glClear (GL_COLOR_BUFFER_BIT);
		glUseProgram (m_SquareShaderProgID);
		glBindVertexArray (m_QuadVA);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, iteratePosn % 2 ? m_ComputeShaderOutputTex2 : m_ComputeShaderOutputTex);
		glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}
}