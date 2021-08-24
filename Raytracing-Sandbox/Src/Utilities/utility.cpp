#include "utility.h"
#include <GLCore.h>
#include <GLCoreUtils.h>
#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <stb_image/stb_image.h>
#include <imgui/imgui_internal.h>

namespace Helper
{
	static GLuint CompileShaderFrom_cstr (const char *source, GLenum shaderTyp)
	{
		GLuint shader = glCreateShader (shaderTyp);

		glShaderSource (shader, 1, &source, NULL);
		glCompileShader (shader);

		GLint isCompiled = 0;
		glGetShaderiv (shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog (maxLength);
			glGetShaderInfoLog (shader, maxLength, &maxLength, &infoLog[0]);

			glDeleteShader (shader);

			LOG_ERROR ("{0}", infoLog.data ());
			// HZ_CORE_ASSERT(false, "Shader compilation failure!");
		}

		return shader;
	}
	std::optional<GLuint> SHADER::CreateProgram (const char *source, GLenum shaderTyp)
	{
		GLuint program = glCreateProgram ();

		GLuint theShader = CompileShaderFrom_cstr (source, shaderTyp);
		glAttachShader (program, theShader);

		glLinkProgram (program);

		GLint isLinked = 0;
		glGetProgramiv (program, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv (program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog (maxLength);
			glGetProgramInfoLog (program, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram (program);

			glDeleteShader (theShader);

			LOG_ASSERT (false, infoLog.data ());
			// HZ_CORE_ASSERT(false, "Shader link failure!");
			return {};
		}

		glDetachShader (program, theShader);
		glDeleteShader (theShader);

		return { program };
	}
	std::optional<GLuint> SHADER::CreateProgram (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2)
	{
		GLuint program = glCreateProgram ();

		GLuint theShader_1 = CompileShaderFrom_cstr (source1, shaderTyp1);
		glAttachShader (program, theShader_1);
		GLuint theShader_2 = CompileShaderFrom_cstr (source2, shaderTyp2);
		glAttachShader (program, theShader_2);

		glLinkProgram (program);

		GLint isLinked = 0;
		glGetProgramiv (program, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv (program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog (maxLength);
			glGetProgramInfoLog (program, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram (program);

			glDeleteShader (theShader_1);
			glDeleteShader (theShader_2);

			LOG_ERROR ("{0}", infoLog.data ());
			// HZ_CORE_ASSERT(false, "Shader link failure!");
			return {};
		}

		glDetachShader (program, theShader_1);
		glDetachShader (program, theShader_2);
		glDeleteShader (theShader_1);
		glDeleteShader (theShader_2);

		return { program };
	}
	std::optional<GLuint> SHADER::CreateProgram (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2, const char *source3, GLenum shaderTyp3)
	{
		GLuint program = glCreateProgram ();

		GLuint theShader_1 = CompileShaderFrom_cstr (source1, shaderTyp1);
		glAttachShader (program, theShader_1);
		GLuint theShader_2 = CompileShaderFrom_cstr (source2, shaderTyp2);
		glAttachShader (program, theShader_2);
		GLuint theShader_3 = CompileShaderFrom_cstr (source3, shaderTyp3);
		glAttachShader (program, theShader_3);

		glLinkProgram (program);

		GLint isLinked = 0;
		glGetProgramiv (program, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv (program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog (maxLength);
			glGetProgramInfoLog (program, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram (program);

			glDeleteShader (theShader_1);
			glDeleteShader (theShader_2);
			glDeleteShader (theShader_3);

			LOG_ERROR ("{0}", infoLog.data ());
			// HZ_CORE_ASSERT(false, "Shader link failure!");
			return {};
		}

		glDetachShader (program, theShader_1);
		glDetachShader (program, theShader_2);
		glDetachShader (program, theShader_3);
		glDeleteShader (theShader_1);
		glDeleteShader (theShader_2);
		glDeleteShader (theShader_3);

		return { program };
	}

	static std::pair<GLenum, GLenum> NumOfIncomingChannelsToIncomingAndInternalFormat (uint8_t numOfChannels)
	{
		GLenum internalFormat = 0, dataFormat = 0;
		if (numOfChannels == 4) {
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		} else if (numOfChannels == 3) {
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}
		LOG_ASSERT (internalFormat & dataFormat, "Format not supported");
		return { dataFormat, internalFormat };
	}
	void TEXTURE_2D::SetData(GLuint ID, uint32_t width, uint32_t height, GLenum src_format, GLenum src_type, const void *data, uint8_t level)
	{

		// Backup GL state
		GLenum last_active_texture; glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint *)&last_active_texture);
		glActiveTexture (GL_TEXTURE0);
		GLuint last_texture; glGetIntegerv (GL_TEXTURE_BINDING_2D, (GLint *)&last_texture);

		if (last_texture != ID)
			glBindTexture (GL_TEXTURE_2D, ID);
		glTexSubImage2D (GL_TEXTURE_2D, level, 0, 0, width, height, src_format, src_type, data);

		// Restore modified GL state
		if (last_texture != ID)
			glBindTexture (GL_TEXTURE_2D, last_texture);
		glActiveTexture (last_active_texture);
	}
	GLuint TEXTURE_2D::Upload (const uint8_t *data, uint32_t width, uint32_t height, uint8_t channels)
	{
		auto [incomingFormat, internalFormat] = NumOfIncomingChannelsToIncomingAndInternalFormat (channels);
		return Upload (data, width, height, internalFormat, incomingFormat, GL_UNSIGNED_BYTE, GL_LINEAR, GL_NEAREST);

	}
	GLuint TEXTURE_2D::Upload (const void *data, uint32_t width, uint32_t height, GLenum internal_format, GLenum src_format, GLenum src_type, GLenum min_filter, GLenum mag_filter)
	{
		GLuint ID;
		glGenTextures (1, &ID);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, ID);

		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

		/// glTexStorage2D is: [it's not working]
		//for (i = 0; i < levels; i++) { // levels = mipMap_Level
		//	glTexImage2D(target, i, internalformat, width, height, 0, format, type, NULL);
		//	width =  max(1, (width / 2));
		//	height = max(1, (height / 2));
		//}

		/* glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height); */
		glTexImage2D (GL_TEXTURE_2D, 0, internal_format, width, height, 0, src_format, src_type, nullptr);
		
		if (data) {
			SetData (ID, width, height, src_format, src_type, data, 0);
			glGenerateMipmap (GL_TEXTURE_2D);
		}
		
		glBindTexture (GL_TEXTURE_2D, 0);
		return ID;

	}
	std::optional<std::tuple<GLuint, uint32_t, uint32_t>> TEXTURE_2D::LoadFromDiskToGPU ()
	{	
		stbi_uc *texData; int width, height, channels;
		stbi_set_flip_vertically_on_load (1);
		texData = stbi_load (GLCore::Utils::FileDialogs::OpenFile("Image\0*.jpeg\0*.png\0*.bmp\0*.hdr\0*.psd\0*.tga\0*.gif\0*.pic\0*.psd\0*.pgm\0").c_str (), &width, &height, &channels, 0);
		if (!texData) {
			LOG_ASSERT (texData, "Failed to load Image");
			return {};
		}

		GLuint textureID = Upload (texData, (uint32_t)width, (uint32_t)height, channels);
		if (textureID > 0)
		{
			return { {textureID, (uint32_t)width, (uint32_t)height} };
		} else {
			return {};
		}
	}
	std::optional<std::tuple<GLuint, glm::uint32_t, glm::uint32_t>> TEXTURE_2D::LoadFromDiskToGPU (const char *location)
	{
		stbi_uc *texData; int width, height, channels;
		stbi_set_flip_vertically_on_load (1);
		texData = stbi_load (location, &width, &height, &channels, 0);
		if (!texData) {
			return LoadFromDiskToGPU ();
		}

		GLuint textureID = Upload (texData, (uint32_t)width, (uint32_t)height, channels);
		if (textureID > 0) {
			return { {textureID, (uint32_t)width, (uint32_t)height} };
		} else {
			return {};
		}
	}

	namespace MATH
	{
		glm::mat3 MakeRotationX (float radians)
		{
			float c = cos (radians);
			float s = sin (radians);
			return (glm::mat3 (1.0f, 0.0f, 0.0f,
							   0.0f, c,   -s,
							   0.0f, s,    c));
		}

		glm::mat3 MakeRotationY (float radians)
		{
			float c = cos (radians);
			float s = sin (radians);
			return (glm::mat3 (c,    0.0f, s,
							   0.0f, 1.0f, 0.0f,
							  -s,    0.0f, c));
		}
		glm::mat3 MakeRotationZ (float radians)
		{
			float c = cos (radians);
			float s = sin (radians);
			return (glm::mat3 (c,   -s,    0.0f,
							   s,    c,    0.0f,
							   0.0f, 0.0f, 1.0f));
		}
	}
	std::string ReadFileAsString (const char *filepath, char ignore_until)
	{
		std::string result;
		std::ifstream in (filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in) {
			if(ignore_until)
				in.ignore (256, ignore_until);
			int start_at = (int)in.tellg () - 1;
			in.seekg (0, std::ios::end);
			size_t size = size_t(in.tellg ()) - start_at;
			if (size != -1) {
				result.resize (size);
				in.seekg (start_at, std::ios::beg);
				in.read (&result[0], size);
			} else {
				LOG_ERROR ("Could not read from file '{0}'", filepath);
			}
		} else {
			LOG_ERROR ("Could not open file '{0}'", filepath);
		}
		return result;
	}
	bool IMGUI::DrawVec3Control (const char *label, glm::vec3 &values, float resetValue /*= 0.0f*/, float columnWidth /*= 100.0f*/)
	{
		ImGuiIO &io = ImGui::GetIO ();
		auto boldFont = io.Fonts->Fonts[0];
		bool status = false;

		ImGui::PushID (label);

		ImGui::Columns (2);
		ImGui::SetColumnWidth (0, columnWidth);
		ImGui::Text (label);
		ImGui::NextColumn ();

		ImGui::PushMultiItemsWidths (3, ImGui::CalcItemWidth ());
		ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor (ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor (ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor (ImGuiCol_ButtonActive, ImVec4{ 0.4f, 0.05f, 0.075f, 1.0f });
		ImGui::PushFont (boldFont);
		if (ImGui::Button ("X", buttonSize))
			status = true, values.x = resetValue;
		ImGui::PopFont ();
		ImGui::PopStyleColor (3);

		ImGui::SameLine ();
		status |= ImGui::DragFloat ("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth ();
		ImGui::SameLine ();

		ImGui::PushStyleColor (ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor (ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor (ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.35f, 0.1f, 1.0f });
		ImGui::PushFont (boldFont);
		if (ImGui::Button ("Y", buttonSize))
			status = true, values.y = resetValue;
		ImGui::PopFont ();
		ImGui::PopStyleColor (3);

		ImGui::SameLine ();
		status |= ImGui::DragFloat ("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth ();
		ImGui::SameLine ();

		ImGui::PushStyleColor (ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor (ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor (ImGuiCol_ButtonActive, ImVec4{ 0.05f, 0.125f, 0.4f, 1.0f });
		ImGui::PushFont (boldFont);
		if (ImGui::Button ("Z", buttonSize))
			status = true, values.z = resetValue;
		ImGui::PopFont ();
		ImGui::PopStyleColor (3);

		ImGui::SameLine ();
		status |= ImGui::DragFloat ("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth ();

		ImGui::PopStyleVar ();

		ImGui::Columns (1);

		ImGui::PopID ();

		return status;
	}

	// ADDING as a node has to wait until later
	namespace Noise
	{

	#define fastFloor(x) ((int (x) < x) ? int (x) : int (x) - 1)

		// Static data
	/*
	 * Permutation table. This is just a random jumble of all numbers 0-255
	 * This needs to be exactly the same for all instances on all platforms,
	 * so it's easiest to just keep it as static explicit data.
	 * This also removes the need for any initialization of this class.
	 */
		std::array<uint8_t, 256> perm = { 151, 160, 137, 91, 90, 15,
		131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
		190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
		88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
		77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
		102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
		135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
		5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
		223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
		129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
		49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
		138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 };

		static float grad2 (uint8_t hash, float x, float y)
		{
			uint8_t h = hash & 7; // Convert low 3 bits of hash code
			float u = y;
			float v = 2 * x;
			if (h < 4) {
				u = x;
				v = 2*y;
			}
			// into 8 simple gradient directions,
			// and compute the dot product with (x,y).
			if (h & 1 != 0) {
				u = -u;
			}
			if (h&2 != 0) {
				v = -v;
			}
			return (u + v);
		}

		// NOISE FUNCTIONS
		// 2D simplex noise
		float Snoise2 (float x, float y)
		{
			const float F2 = 0.366025403; // F2 = 0.5*(sqrt(3.0)-1.0)
			const float G2 = 0.211324865; // G2 = (3.0-Math.sqrt(3.0))/6.0

			float n0 = 0, n1 = 0, n2 = 0; // Noise contributions from the three corners

			// Skew the input space to determine which simplex cell we're in
			float s = (x + y) * F2; // Hairy factor for 2D
			float xs = x + s;
			float ys = y + s;
			int i = fastFloor (xs);
			int j = fastFloor (ys);

			float t = float (i+j) * G2;
			float X0 = float (i) - t; // Un-skew the cell origin back to (x,y) space
			float Y0 = float (j) - t;
			float x0 = x - X0; // The x,y distances from the cell origin
			float y0 = y - Y0;

			// For the 2D case, the simplex shape is an equilateral triangle.
			// Determine which simplex we are in.
			uint8_t i1 = 0, j1 = 0; // Offsets for second (middle) corner of simplex in (i,j) coords
			if (x0 > y0) {
				i1 = 1;
				j1 = 0;
			} else { // lower triangle, XY order: (0,0)->(1,0)->(1,1)
				i1 = 0;
				j1 = 1;
			} // upper triangle, YX order: (0,0)->(0,1)->(1,1)

			// A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
			// a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
			// c = (3-sqrt(3))/6

			float x1 = x0 - float (i1) + G2; // Offsets for middle corner in (x,y) Un-skewed coords
			float y1 = y0 - float (j1) + G2;
			float x2 = x0 - 1.0 + 2.0*G2; // Offsets for last corner in (x,y) Un-skewed coords
			float y2 = y0 - 1.0 + 2.0*G2;

			// Wrap the integer indices at 256, to avoid indexing perm[] out of bounds
			uint8_t ii = uint8_t (i);
			uint8_t jj = uint8_t (j);

			// Calculate the contribution from the three corners
			float t0 = 0.5 - x0*x0 - y0*y0;
			if (t0 < 0.0) {
				n0 = 0.0;
			} else {
				t0 *= t0;

				uint8_t _tmp = jj;
				_tmp = ii+perm[_tmp];
				n0 = t0 * t0 * grad2 (perm[_tmp], x0, y0);
			}

			float t1 = 0.5 - x1*x1 - y1*y1;
			if (t1 < 0.0) {
				n1 = 0.0;
			} else {
				t1 *= t1;

				uint8_t _tmp = jj+j1;
				_tmp = ii+i1+perm[_tmp];
				n1 = t1 * t1 * grad2 (perm[_tmp], x1, y1);
			}

			float t2 = 0.5 - x2*x2 - y2*y2;
			if (t2 < 0.0) {
				n2 = 0.0;
			} else {
				t2 *= t2;

				uint8_t _tmp = jj+1;
				_tmp = ii+1+perm[_tmp];
				n2 = t2 * t2 * grad2 (perm[_tmp], x2, y2);
			}

			// Add contributions from each corner to get the final noise value.
			return (n0 + n1 + n2);
		}


		//Turbulance is turbulant fractal type noise
		float Turbulance (float x, float y, float freq, float lac, float gain, int octaves)
		{
			float sum = 0, f = 0;
			float ampltude = 1.0f;
			for (uint32_t i = 0; i < octaves; i++) {
				f = Snoise2 (x*freq, y*freq) * ampltude;
				if (f < 0) {
					f = -f;
				}
				sum += f;
				freq *= lac;
				ampltude *= gain;
			}
			return sum;
		}

		//Fbm2 is Fractal Brownim Motion type noise
		float Fbm2 (float x, float y, float freq, float lac, float gain, int octaves)
		{
			float sum = 0;
			float ampltude = 1.0f;
			for (uint32_t i = 0; i < octaves; i++) {
				sum += Snoise2 (x*freq, y*freq) * ampltude;
				freq *= lac;
				ampltude *= gain;
			}
			return sum;
		}
	};
}