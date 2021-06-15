#pragma once

#include <GLCore.h>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "Utilities/utility.h"

class ComputeAndSqrShader_Base: public GLCore::TestBase
{
public:
	ComputeAndSqrShader_Base (const char* name = "Compute & Sqr Shader Base", const char* discription = "just a base"
							 , const char* default_compute_shader_src = s_default_compute_shader
							 , const char* default_sqr_shader_vert_src = s_default_sqr_shader_vert
							 , const char* default_sqr_shader_frag_src = s_default_sqr_shader_frag
	);
	virtual ~ComputeAndSqrShader_Base () = default;

	virtual void OnDetach         () = 0;
	virtual void OnAttach         () = 0;
	virtual void OnUpdate (GLCore::Timestep ts) = 0;
	virtual void OnImGuiRender    () = 0;
	virtual void ImGuiMenuOptions () = 0;

	virtual void OnComputeShaderReload () {};
	virtual void OnSquareShaderReload  () {};
protected:
	void OnImGuiComputeShaderSource ()
	{
		ImVec2 contentRegion = ImGui::GetContentRegionAvail ();
		ImVec2 textbox_contentRegion (contentRegion.x, contentRegion.y - 60);
		ImVec2 button_contentRegion (contentRegion.x, 50);

		ImGui::InputTextMultiline ("Compute Shader Source: ", m_ComputeShaderTXT.raw_data (), m_ComputeShaderTXT.size (), textbox_contentRegion);
		if (ImGui::Button ("Reload Compute Shader", button_contentRegion))
			ReloadComputeShader ();
	}
	void OnImGuiSqureShaderSource ()
	{
		ImVec2 contentRegion = ImGui::GetContentRegionAvail ();
		contentRegion.y /= 2.0f;
		ImVec2 textbox_contentRegion (contentRegion.x, contentRegion.y - 60);
		ImVec2 button_contentRegion (contentRegion.x, 60);

		ImGui::Text ("Vertex SRC:");
		ImGui::InputTextMultiline ("Vert SRC", m_SquareShaderTXT_vert.raw_data (), m_SquareShaderTXT_vert.size (), textbox_contentRegion);

		ImGui::Separator ();

		ImGui::Text ("Fragment SRC:");
		ImGui::InputTextMultiline ("Frag SRC", m_SquareShaderTXT_frag.raw_data (), m_SquareShaderTXT_frag.size (), textbox_contentRegion);

		if (ImGui::Button ("Reload Square Shader", button_contentRegion))
			ReloadSquareShader ();
	}
	void ReGenQuadVAO (glm::vec2 &min = glm::vec2(-1, -1),glm::vec2 &max = glm::vec2 (1, 1))
	{
		DeleteQuadVAO();
		
		glGenVertexArrays (1, &m_QuadVA);
		glBindVertexArray (m_QuadVA);

		float vertices[] = {
			min.x, min.y, 0.0f,
			max.x, min.y, 0.0f,
			max.x, max.y, 0.0f,
			min.x, max.y, 0.0f
		};
		uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };

		glGenBuffers (1, &m_QuadVB);
		glBindBuffer (GL_ARRAY_BUFFER, m_QuadVB);
		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof (float) * 3, 0);// positions

		glGenBuffers (1, &m_QuadIB);
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_QuadIB);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);
	}
	void DeleteQuadVAO ()
	{
		if (m_QuadVA) {
			glDeleteVertexArrays (1, &m_QuadVA);
			m_QuadVA = 0;
		}
		if (m_QuadVB) {
			glDeleteBuffers (1, &m_QuadVB);
			m_QuadVB = 0;
		}
		if (m_QuadIB) {
			glDeleteBuffers (1, &m_QuadIB);
			m_QuadIB = 0;
		}
	}
	void ReloadComputeShader ()
	{
		std::optional<GLuint> shader_program = Helper::SHADER::CreateProgram (m_ComputeShaderTXT.data (), GL_COMPUTE_SHADER);
		if (shader_program.has_value ()) {
			DeleteComputeShader ();
			{// check whether last bound program is this, if yes then update it
				GLuint last_program;
				glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *)&last_program);
				if (m_ComputeShaderProgID == last_program)
					glUseProgram (shader_program.value ());
			}
			m_ComputeShaderProgID = shader_program.value ();
			OnComputeShaderReload ();
		}
	}
	void DeleteComputeShader ()
	{
		if (m_ComputeShaderProgID) {
			glDeleteProgram (m_ComputeShaderProgID);
			m_ComputeShaderProgID = 0;
		}
	}
	void ReloadSquareShader ()
	{
		std::optional<GLuint> shader_program = Helper::SHADER::CreateProgram (m_SquareShaderTXT_vert.data (), GL_VERTEX_SHADER, m_SquareShaderTXT_frag.data (), GL_FRAGMENT_SHADER);
		if (shader_program.has_value ()) {
			DeleteSquareShader ();
			{// check whether last bound program is this, if yes then update it
				GLuint last_program;
				glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *)&last_program);
				if (m_SquareShaderProgID == last_program)
					glUseProgram (shader_program.value ());
			}
			m_SquareShaderProgID = shader_program.value ();
			OnSquareShaderReload ();
		}
	}
	void DeleteSquareShader ()
	{
		if (m_SquareShaderProgID) {
			glDeleteProgram (m_SquareShaderProgID);
			m_SquareShaderProgID = 0;
		}
	}

	//////////////
	// Structures
private:
	struct Buffer
	{
	public:
		static Buffer Create (const char *default_data, const uint16_t min_size = 1024)
		{
			uint16_t size_str = 0;
			while (default_data[size_str] != '\0')
				size_str++;

			const uint16_t new_size_str = (size_str > min_size ? size_str + 100 : min_size);

			char *data = new char[new_size_str];
			size_str++; // copy '\0'
			for (uint16_t i = 0; i < size_str; i++) {
				data[i] = default_data[i];
			}
			return Buffer (data, new_size_str);
		}
		Buffer (const uint16_t size = 1537)
			:_size (size)
		{
			_data = new char[size];
			_data[0] = '\0';
		}
		~Buffer ()
		{
			delete _data;
			_data = 0;
		}
		const char *data () const { return _data; }
		char *raw_data () { return _data; }
		const uint16_t size () const { return _size; }
	private:
		Buffer (char *data, const uint16_t size = 1537)
			:_data (data), _size (size)
		{}
	private:
		char *_data;
		const uint16_t _size;
	};
	//////////////
	// Variables
protected:
	static const char *s_default_compute_shader;
	static const char *s_default_sqr_shader_vert;
	static const char *s_default_sqr_shader_frag;
	GLuint m_ComputeShaderProgID = 0, m_SquareShaderProgID = 0;
	GLuint m_QuadVA = 0, m_QuadVB = 0, m_QuadIB = 0;
private:
	Buffer m_ComputeShaderTXT, m_SquareShaderTXT_vert, m_SquareShaderTXT_frag;
};