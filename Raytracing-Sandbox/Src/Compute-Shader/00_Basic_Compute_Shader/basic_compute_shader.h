#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>
#include <optional>
#include <glm/glm.hpp>

class BasicComputeShader_Test: public GLCore::TestBase
{
public:
	BasicComputeShader_Test (const char* name = "Basic Compute Shader", const char* discription = "creating a basic compute shader which sets each pixel and displays it on-to screen acc to Square_Shader"
							 , const char* default_compute_shader_src = s_default_compute_shader
							 , const char* default_sqr_shader_vert_src = s_default_sqr_shader_vert
							 , const char* default_sqr_shader_frag_src = s_default_sqr_shader_frag
	);
	virtual ~BasicComputeShader_Test () = default;

	virtual void OnAttach () override;
	virtual void OnDetach () override;
	virtual void OnUpdate (GLCore::Timestep ts) override;
	virtual void OnImGuiRender () override;
	virtual void ImGuiMenuOptions () override;

	virtual void OnAttachExtras () {};
	virtual void OnDetachExtras () {};
	virtual void OnImGuiRenderUnderSettingsTab () {};
private:
	void ReloadComputeShader ();
	void ReloadSquareShader ();
protected:
	static const char *s_default_compute_shader;
	static const char *s_default_sqr_shader_vert;
	static const char *s_default_sqr_shader_frag;
	GLuint m_ComputeShaderOutputTex = 0;
	GLuint m_ComputeShaderProgID = 0, m_SquareShaderProgID = 0;
	GLuint m_QuadVA = 0, m_QuadVB = 0, m_QuadIB = 0;
	const glm::ivec2 m_OutputTexDimensions = glm::ivec2 (512, 512);
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
			delete[] _data;
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
	Buffer m_ComputeShaderTXT, m_SquareShaderTXT_vert, m_SquareShaderTXT_frag;
};