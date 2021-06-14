#include "base.h"

using namespace GLCore;
using namespace GLCore::Utils;


const char *ComputeAndSqrShader_Base::s_default_compute_shader = R"(
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
const char *ComputeAndSqrShader_Base::s_default_sqr_shader_vert = R"(
#version 440 core
layout (location = 0) in vec3 in_Position;

layout (location = 0) out vec2 b_Coord;

void main()
{
	gl_Position = vec4(in_Position, 1.0f);
	b_Coord = vec2(in_Position.x > 0 ? 1.0 : 0, in_Position.y > 0 ? 1.0 : 0);
})";
const char *ComputeAndSqrShader_Base::s_default_sqr_shader_frag = R"(
#version 440 core
layout (location = 0) in vec2 b_Coord;
layout (rgba32f, binding = 0) uniform sampler2D u_Texture;

layout (location = 0) out vec4 o_Color;

void main()
{
	o_Color = texture(u_Texture, b_Coord);
})";

ComputeAndSqrShader_Base::ComputeAndSqrShader_Base(const char* name , const char* discription
	, const char* default_compute_shader_src , const char *default_sqr_shader_vert_src , const char *default_sqr_shader_frag_src 
) : TestBase (std::string(name), std::string(discription))
	, m_ComputeShaderTXT     (Buffer::Create (default_compute_shader_src))
	, m_SquareShaderTXT_vert (Buffer::Create (default_sqr_shader_vert_src, 512))
	, m_SquareShaderTXT_frag (Buffer::Create (default_sqr_shader_frag_src, 512))
{}