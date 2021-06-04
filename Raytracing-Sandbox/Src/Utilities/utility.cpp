#include "utility.h"
#include <GLCore.h>
#include <GLCoreUtils.h>
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
	void TEXTURE_2D::SetData (GLuint ID, uint32_t width, uint32_t height, GLenum incomingFormat, const uint8_t *data, uint8_t level)
	{

		// Backup GL state
		GLenum last_active_texture; glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint *)&last_active_texture);
		glActiveTexture (GL_TEXTURE0);
		GLuint last_texture; glGetIntegerv (GL_TEXTURE_BINDING_2D, (GLint *)&last_texture);

		if (last_texture != ID)
			glBindTexture (GL_TEXTURE_2D, ID);
		glTexSubImage2D (GL_TEXTURE_2D, level, 0, 0, width, height, incomingFormat, GL_UNSIGNED_BYTE, data);

		// Restore modified GL state
		if (last_texture != ID)
			glBindTexture (GL_TEXTURE_2D, last_texture);
		glActiveTexture (last_active_texture);
	}
	GLuint TEXTURE_2D::Upload (const uint8_t *data, uint32_t width, uint32_t height, uint8_t channels)
	{
		auto [incomingFormat, internalFormat] = NumOfIncomingChannelsToIncomingAndInternalFormat (channels);

		GLuint ID;
		glGenTextures (1, &ID);
		glBindTexture (GL_TEXTURE_2D, ID);

		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		/// glTexStorage2D is: [it's not working]
		//for (i = 0; i < levels; i++) { // levels = mipMap_Level
		//	glTexImage2D(target, i, internalformat, width, height, 0, format, type, NULL);
		//	width =  max(1, (width / 2));
		//	height = max(1, (height / 2));
		//}

		/* glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height); */
		glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, width, height, 0, incomingFormat, GL_UNSIGNED_BYTE, nullptr);
		SetData (ID, width, height, incomingFormat, data, 0);
		glGenerateMipmap (GL_TEXTURE_2D);
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
}