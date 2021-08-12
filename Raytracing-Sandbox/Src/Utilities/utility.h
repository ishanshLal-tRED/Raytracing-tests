#pragma once
#include <optional>
#include <tuple>
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>

#define MIN(x,y) (x > y ? y :  x)
#define MAX(x,y) (x > y ? x :  y)
#define ABS(x)   (x > 0 ? x : -x)
#define MOD(x,y) (x - ((int)(((float)x) / y) * y))

namespace Helper
{
	// delete after use
	std::string ReadFileAsString (const char *filepath, char ignore_until = 0);
	namespace SHADER
	{
		std::optional<GLuint> CreateProgram (const char *source, GLenum shaderTyp);
		std::optional<GLuint> CreateProgram (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2);
		std::optional<GLuint> CreateProgram (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2, const char *source3, GLenum shaderTyp3);
	}
	namespace TEXTURE_2D
	{
		void SetData (GLuint ID, uint32_t width, uint32_t height, GLenum src_format, GLenum src_type, const void *data, uint8_t level);
		GLuint Upload (const uint8_t *data, uint32_t width, uint32_t height, uint8_t channels);
		GLuint Upload (const void *data, uint32_t width, uint32_t height, GLenum internal_format, GLenum src_format, GLenum src_type, GLenum min_filter = GL_NEAREST, GLenum mag_filter = GL_NEAREST);
		std::optional<std::tuple<GLuint, uint32_t, uint32_t>> LoadFromDiskToGPU ();
		std::optional<std::tuple<GLuint, uint32_t, uint32_t>> LoadFromDiskToGPU (const char *location);
	}
	namespace MATH
	{
		glm::mat3 MakeRotationX (float radians);
		glm::mat3 MakeRotationY (float radians);
		glm::mat3 MakeRotationZ (float radians);
	}
	namespace IMGUI
	{
		bool DrawVec3Control (const char *label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = 100.0f);
	}
}