#pragma once
#include <optional>
#include <glad/glad.h>

namespace Helper
{
	std::optional<GLuint> GenerateShaderProg (const char *source, GLenum shaderTyp);
	std::optional<GLuint> GenerateShaderProg (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2);
	std::optional<GLuint> GenerateShaderProg (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2, const char *source3, GLenum shaderTyp3);
}