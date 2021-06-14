#include "test.h"
#include <glm/glm.hpp>
#include <thread>
#include <chrono>

// TODO since we cannot create a larger Equation array, try splitting data into 2 array // Edit: cannot be done through normal means
const char *just_a_test::s_just_a_compute_shader = R"(

#version 440 core
layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D img_output1;
//layout (rgba32f, binding = 1) uniform image2D img_output2;
//layout (rgba32f, binding = 2) uniform image2D img_output3;
//layout (rgba32f, binding = 3) uniform image2D img_output4;

layout (r32f, location = 0) uniform sampler2D u_Texture;
uniform int u_Index;
const ivec2 tex_size = textureSize (u_Texture, 0);
//
const mediump int Node_OpenBrace       = -1;
const mediump int Node_CloseBrace      = -2;
const mediump int Node_pseudo_constant = -3;
const mediump int Node_Max             = 1 ;
const mediump int Node_Min             = Node_Max   + 1;
const mediump int Node_arcTan2         = Node_Min   + 1;
const mediump int Node_Sin             = Node_arcTan2 + 1;
const mediump int Node_Cos             = Node_Sin   + 1;
const mediump int Node_Tan             = Node_Cos   + 1;
const mediump int Node_aTan            = Node_Tan   + 1;
const mediump int Node_Lerp            = Node_aTan  + 1;
const mediump int Node_Plus            = Node_Lerp  + 1;
const mediump int Node_Minus           = Node_Plus  + 1;
const mediump int Node_Mult            = Node_Minus + 1;
const mediump int Node_Div             = Node_Mult  + 1;
const mediump int Node_Negate          = Node_Div   + 1;
const mediump int Node_Square          = Node_Negate+ 1;
const mediump int Node_Ceil            = Node_Square+ 1;
const mediump int Node_Log2            = Node_Ceil  + 1;
const mediump int Node_Abs             = Node_Log2  + 1;
const mediump int Node_Clip            = Node_Abs   + 1;
const mediump int Node_Floor           = Node_Clip  + 1;
const mediump int Node_Wrap            = Node_Floor + 1;
const mediump int Node_Const           = Node_Wrap  + 1;
const mediump int Node_OpX             = Node_Const + 1;
const mediump int Node_OpY             = Node_OpX   + 1;
bool TypeHasData (int i)
{
	switch (i) {
	case Node_Const:
		return true;
	case Node_OpX:
		return true;
	case Node_OpY:
		return true;
	default:
		return false;
	}
}
float Evaluate (int type, float x, float y, float z, float constant)
{
	switch (type) {
		case Node_Max    :
			return (x > y ? x : y);
		case Node_Min    :
			return (x < y ? x : y);
		case Node_arcTan2:
			return atan (x, y);
		case Node_Sin    :
			return sin  (radians(x));
		case Node_Cos    :
			return cos  (radians(x));
		case Node_Tan    :
			return tan  (radians(x));
		case Node_aTan   :
			return atan (radians(x));
		case Node_Lerp   :
			return x + z*(y - x);
		case Node_Plus   :
			return x + y;
		case Node_Minus  :
			return x - y;
		case Node_Mult   :
			return x * y;
		case Node_Div    :
			return x / y;
		case Node_Negate :
			return -x;
		case Node_Square :
			return x*x;
		case Node_Ceil   :
			return float(ceil(x));
		case Node_Log2   :
			return log2(x);
		case Node_Abs   :
			return abs(x);
		case Node_Clip   :
			return x > y ? y : (x < -y ? -y : x);
		case Node_Floor  :
			return floor(x);
		case Node_Wrap   :
			return -1 + 2*(((x + 1)/2) - floor(((x + 1)/2)));
		case Node_Const  :
			return constant;
		case Node_OpX    :
			return x;
		case Node_OpY    :
			return y;
		case Node_pseudo_constant: // pseudo constant
			return constant;
		default:
			return 0;
	}
}
float DataFromTexture (int pos_x, int pos_y)
{
	float data = texture (u_Texture, vec2 (pos_x / float (tex_size.x - 1), pos_y / float (tex_size.y - 1))).r;
	return data;
}
mediump int Eqn[40]; // size should have been = tex_size.x
// mediump int Eqn_pt2[40];
int EqnAt(int index){
	return Eqn[index];// (index < Eqn.length() ? Eqn[index] : Eqn_pt2[index - Eqn.length()]);
}
void EqnSetAt(int index, int data){
	Eqn[index] = data;//index < Eqn.length() ? (Eqn[index] = data) : (Eqn_pt2[index - Eqn.length()] = data);
}
float Eval (float x, float y, const int level)
{
	mediump int Eqn_Size = 0;
    //i = (Eqn.length() + Eqn_pt2.length()) - 1
	for(int i = Eqn.length() - 1; i >= 0; i--){
		int incomingType = int(DataFromTexture (i, level));
		EqnSetAt(i, incomingType);
        if(Eqn_Size == 0 && incomingType != 0)
             Eqn_Size = i+1;
	}
	float constant = 0;
	float pseudo_constant_val[3] = float[3](0,0,0);
	mediump int front_index = 1;
	mediump int back_index = 0;
	mediump int pseudo_Eval_pos = -1; // position from where push pop has to be done, more like No. of pseudo_constant Evaluated before this scope
	mediump int data_index = 0;
	// if only one leaf operator
	if(EqnAt(1) == 0){
		if(TypeHasData(EqnAt(0))){
			constant = DataFromTexture(data_index, level + 1);
			data_index++;
		}
		return Evaluate(EqnAt(0), x, y, 0, constant);
	}

	while(front_index > 0){
		// evaluate scope
		back_index = front_index;
		while (EqnAt(back_index) != Node_CloseBrace)
			back_index++;
		pseudo_Eval_pos++; // we shift a position
		front_index = back_index;
		while (EqnAt(front_index) != Node_OpenBrace){
			if(EqnAt(front_index) == Node_pseudo_constant)
				pseudo_Eval_pos--;
			front_index--;
		}
		// Solve scope
		{
			float results[3] = float[3](0,0,0);
			for (int index = front_index + 1, i = 0; index < back_index; index++, i++){
				int type = EqnAt(index);
				if(TypeHasData(type)){
					constant = DataFromTexture(data_index, level + 1);
					data_index++;
				}
				if(type == Node_pseudo_constant){
					constant = pseudo_constant_val[pseudo_Eval_pos];
					// used so pop front
					for(int i = pseudo_Eval_pos; i < 2; i++){
						pseudo_constant_val[i] = pseudo_constant_val[i + 1];
					}
					pseudo_constant_val[2] = 0;
				}
				results[i] = Evaluate(type, x, y, 0, constant);
				constant = 0;
			}
			front_index--; // operator
			int type = EqnAt(front_index);
			if(TypeHasData(type)){
				constant = DataFromTexture(data_index, level + 1);
				data_index++;
			}
			// Evaluating so push front
			for(int i = pseudo_Eval_pos; i < 2; i++){
				pseudo_constant_val[i + 1] = pseudo_constant_val[i];
			}
			pseudo_constant_val[pseudo_Eval_pos] = Evaluate(type, results[0], results[1], results[2], constant);
			// collapse scoped part of Eqn 
			EqnSetAt(front_index, Node_pseudo_constant); // operator evaluated
			for(int i = 1; i < (Eqn_Size - back_index); i++){
				EqnSetAt(i + front_index, EqnAt(i + back_index)); // shift
				EqnSetAt(i + back_index, 0);
			}
            Eqn_Size -= (back_index - front_index);
		}
	}
	return pseudo_constant_val[0];
}
vec4 out_Pixel (ivec2 pixel_coords, ivec2 image_size, const int level){
	//int x = int((float (pixel_coords.x)/(image_size.x-1))*tex_size.x);
	//int y = int((float (pixel_coords.y)/(image_size.y-1))*tex_size.y);

	float r = Eval (float (pixel_coords.x), float (pixel_coords.y), 6*level + 0); // DataFromTexture (x, y);
	float g = Eval (float (pixel_coords.x), float (pixel_coords.y), 6*level + 2);
	float b = Eval (float (pixel_coords.x), float (pixel_coords.y), 6*level + 4);
	return vec4(r, g, b, 1.0);
}
void main ()
{
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2 (gl_GlobalInvocationID.xy);
	
	vec4 pixel;
	switch(int(gl_GlobalInvocationID.z)%4){
		default:
		//case 1:
			// base pixel color for image
			pixel = out_Pixel (pixel_coords, imageSize(img_output1), u_Index);
			// output to a specific pixel in the image
			imageStore (img_output1, pixel_coords, pixel);
			break;
		//case 2:
		//	// base pixel color for image
		//	pixel = out_Pixel (pixel_coords, imageSize(img_output2), 1);
		//	// output to a specific pixel in the image
		//	imageStore (img_output2, pixel_coords, pixel);
		//	break;
		//case 3:
		//	// base pixel color for image
		//	pixel = out_Pixel (pixel_coords, imageSize(img_output3), 2);
		//	// output to a specific pixel in the image
		//	imageStore (img_output3, pixel_coords, pixel);
		//	break;
		//default:
		//	// base pixel color for image
		//	pixel = out_Pixel (pixel_coords, imageSize(img_output4), 4);
		//	// output to a specific pixel in the image
		//	imageStore (img_output4, pixel_coords, pixel);
		//	break;
	}
}
)";
const char *just_a_test::s_just_a_sqr_shader_vert = R"(
#version 440 core
layout (location = 0) in vec3 in_Position;
uniform vec2 u_RelPosn;
uniform float u_Scale;
layout (location = 0) out vec2 b_TexCoord;
layout (location = 1) out vec2 b_Coord;

void main()
{
	vec3 Position = vec3(in_Position.x*(u_Scale/abs(in_Position.x)), in_Position.y * (u_Scale/abs(in_Position.y)), in_Position.z);
	Position.x -= u_RelPosn.x;
	Position.y -= u_RelPosn.y;
	
	gl_Position = vec4(Position, 1.0f);
	b_Coord.x = (Position.x + 1.0)*0.5;
	b_Coord.y = (Position.y + 1.0)*0.5;
	b_TexCoord = vec2(in_Position.x > 0 ? 1.0 : 0, in_Position.y > 0 ? 1.0 : 0);
})";
const char *just_a_test::s_just_a_sqr_shader_frag = R"(
#version 440 core
layout (location = 0) in vec2 b_TexCoord;
layout (location = 1) in vec2 b_Coord;
layout (r32f, binding = 0) uniform image2D img_output;
layout (rgba32f, binding = 0) uniform sampler2D u_Texture;
uniform int u_Index;

layout (location = 0) out vec4 o_Color;

void main()
{
	o_Color = texture(u_Texture, b_TexCoord);
	ivec2 img_size = imageSize(img_output);
    ivec2 pix_coord = ivec2(img_size.x*b_Coord.x,img_size.y*b_Coord.y); 
	imageStore (img_output, pix_coord, vec4(u_Index, 0, 0, 0));
})";
static std::vector<float> DATA;
void just_a_test::ReloadPicture ()
{
	uint32_t y, x;
	// Send eqn
	if (DATA.size () < (m_Eqn_TEX_Width*6*m_GridHeight*m_GridHeight))
		DATA.resize (m_Eqn_TEX_Width*6*m_GridHeight*m_GridHeight);
	for (uint16_t y = 0; y < 6*m_GridHeight*m_GridHeight; y+=6) {
		auto [reds_arr, reds_data]     = pics[y/6].r.TreeToArray ();
		auto [greens_arr, greens_data] = pics[y/6].g.TreeToArray ();
		auto [blues_arr, blues_data]   = pics[y/6].b.TreeToArray ();
		for (uint16_t x = 0; x < m_Eqn_TEX_Width; x++) {
			DATA[(y+0)*m_Eqn_TEX_Width + x] = x < reds_arr.size () ? (float)reds_arr[x] : 0;
			DATA[(y+1)*m_Eqn_TEX_Width + x] = x < reds_data.size () ? reds_data[x] : 0;
			DATA[(y+2)*m_Eqn_TEX_Width + x] = x < greens_arr.size () ? (float)greens_arr[x] : 0;
			DATA[(y+3)*m_Eqn_TEX_Width + x] = x < greens_data.size () ? greens_data[x] : 0;
			DATA[(y+4)*m_Eqn_TEX_Width + x] = x < blues_arr.size () ? (float)blues_arr[x] : 0;
			DATA[(y+5)*m_Eqn_TEX_Width + x] = x < blues_data.size () ? blues_data[x] : 0;
		}
	}
	if (m_EquationsTexture) {
		glDeleteTextures (1, &m_EquationsTexture);
		glDeleteTextures (1, &m_SubstituteTexture);
	}
	m_EquationsTexture = Helper::TEXTURE_2D::Upload (DATA.data (), m_Eqn_TEX_Width, 6*m_GridHeight*m_GridHeight, GL_R32F, GL_RED, GL_FLOAT);
	m_SubstituteTexture = Helper::TEXTURE_2D::Upload (DATA.data (), m_Eqn_TEX_Width, 6*m_GridHeight*m_GridHeight, GL_R32F, GL_RED, GL_FLOAT);
}
void just_a_test::OnUpdate (GLCore::Timestep ts)
{
	glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	static float time = 0;
	time += ts;
	static int index = 0;
	if (m_ComputeAgain) {
		index = 0;
		time = 0;
		m_ComputeAgain = false;
	}
	if (index < pics.size()) {
		glClear (GL_COLOR_BUFFER_BIT);
		glUseProgram (m_ComputeShaderProgID);
		glUniform1i (m_ImgIndexShaderID, index);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, m_EquationsTexture);
		pics[index].BindImg ();
		glDispatchCompute (pics[index].tex_w, pics[index].tex_h, 1);
		
		//// make sure writing to image has finished before read
		//glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		index++;
	}
	if(time < 3.0f)
		LOG_TRACE ("updated");
	// if (time > cube)
	{ // normal drawing pass
		glUseProgram (m_SquareShaderProgID);
		glUniform1f (m_ScaleShaderID, 1.0f/m_GridHeight);
		
		for (uint16_t y = 0; y < m_GridHeight; y++)
		{
			for (uint16_t x = 0; x < m_GridHeight; x++) {
				glUniform2f (m_RelPosnShaderID, y*(2.0f/m_GridHeight) + 1.0f/m_GridHeight - 1.0f, x*(2.0f/m_GridHeight) + 1.0f/m_GridHeight - 1.0f);
				glBindVertexArray (m_QuadVA);
				glBindImageTexture (0, m_OutIndexImg, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
				glActiveTexture (GL_TEXTURE0);
				int i = y*m_GridHeight + x;
				if(i < index - 1 || index == pics.size ())
					glBindTexture (GL_TEXTURE_2D, pics[i].out_img);
				else
					glBindTexture (GL_TEXTURE_2D, m_SubstituteTexture);
				glUniform1i (m_SqrIndexShaderID, i + 1);// Squre no.
				glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			}
		}
	}
}
float *TextureData = new float[100*100];
void just_a_test::OnEvent (GLCore::Event &event)
{
	GLCore::EventDispatcher dispatcher (event);
	dispatcher.Dispatch<GLCore::MouseMovedEvent> (
		[&](GLCore::MouseMovedEvent &e) {
			LOG_TRACE ("mouse: {0}, {1}", e.GetX (), e.GetY ());
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_OutIndexImg);
			glGetTexImage (GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, TextureData);
			m_PixData = TextureData[int (e.GetX ()/m_ViewPortSize.x)*100*100 + int (e.GetY ()/m_ViewPortSize.y)*100];
			return false;
		});
	dispatcher.Dispatch<GLCore::LayerViewportResizeEvent> (
		[&](GLCore::LayerViewportResizeEvent &e) {
			m_ViewPortSize.x = e.GetWidth ();
			m_ViewPortSize.y = e.GetHeight ();
			return false;
		});
}