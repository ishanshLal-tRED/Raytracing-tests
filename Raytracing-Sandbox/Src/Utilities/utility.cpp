#include "utility.h"
#include "GLCore.h"
#include <vector>

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
	std::optional<GLuint> GenerateShaderProg (const char *source, GLenum shaderTyp)
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
	std::optional<GLuint> GenerateShaderProg (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2)
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
	std::optional<GLuint> GenerateShaderProg (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2, const char *source3, GLenum shaderTyp3)
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
}