#include "utility.h"
#include <GLCore.h>
#include <GLCoreUtils.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stb_image/stb_image.h>

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

			LOG_ERROR ("{0}", infoLog.data ());
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
}