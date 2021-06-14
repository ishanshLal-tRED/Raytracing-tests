#pragma once
#include <GLCore.h>
#include "Compute-Shader/00_Basic_Compute_Shader/basic_compute_shader.h"
#include "APT.h"
#include <glad/glad.h>
#include "Utilities/utility.h"

struct Picture
{
	APT::NodeTree r;
	APT::NodeTree g;
	APT::NodeTree b;
	GLuint out_img = 0;
	uint32_t tex_w = 0;
	uint32_t tex_h = 0;
	void ResetImg (uint32_t width, uint32_t height)
	{
		if (tex_w != width || tex_h != height)
			tex_w = width, tex_h = height;
		else
			return;

		if (out_img)
			glDeleteTextures (1, &out_img);

		glGenTextures (1, &out_img);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, out_img);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);

		// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
		glBindImageTexture (0, out_img, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}
	void BindImg (uint32_t at_unit = 0)
	{
		glBindImageTexture (at_unit, out_img, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}
	~Picture ()
	{
		if (out_img) {
			glDeleteTextures (1, &out_img);
			out_img = 0;
			tex_w = 0, tex_h = 0;
		}
	}
};

class just_a_test
	: public BasicComputeShader_Test
{
public:
	just_a_test (const char *name = "just a test for node tree")
		: BasicComputeShader_Test (name, "just a test", s_just_a_compute_shader, s_just_a_sqr_shader_vert, s_just_a_sqr_shader_frag)
	{
		m_OutIndexImg = 0;
	}
	virtual void OnAttachExtras ()
	{
		m_RelPosnShaderID = glGetUniformLocation (m_SquareShaderProgID, "u_RelPosn");
		m_ScaleShaderID = glGetUniformLocation (m_SquareShaderProgID, "u_Scale");
		m_SqrIndexShaderID = glGetUniformLocation (m_SquareShaderProgID, "u_Index");
		m_ImgIndexShaderID = glGetUniformLocation (m_ComputeShaderProgID, "u_Index");
		
		if (m_OutIndexImg)
			glDeleteTextures (1, &m_OutIndexImg);
		m_OutIndexImg = 0;
		glGenTextures (1, &m_OutIndexImg);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, m_OutIndexImg);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D (GL_TEXTURE_2D, 0, GL_R32F, 100, 100, 0, GL_RED, GL_FLOAT, NULL);

		// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
		glBindImageTexture (0, m_OutIndexImg, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
		
		srand ((uint32_t)(GLCore::Application::GetTimeInSeconds()*1000));
		pics.resize (m_GridHeight*m_GridHeight);
		for (Picture& pic: pics) {
			pic.r.ResetCluster ();
			pic.r.SpawnRandomTree (17);
			pic.g.ResetCluster ();
			pic.g.SpawnRandomTree (17);
			pic.b.ResetCluster ();
			pic.b.SpawnRandomTree (17);
			pic.ResetImg (512, 512);
		}
		ReloadPicture ();
		m_ComputeAgain = true;
	}
	virtual void OnDetachExtras () {
		glDeleteTextures (1, &m_EquationsTexture);
		m_EquationsTexture = 0;
		glDeleteTextures (1, &m_SubstituteTexture);
		m_SubstituteTexture = 0;
		pics.clear ();
	}
	void ReloadPicture ();
	virtual void OnImGuiRenderUnderSettingsTab () {
		if (ImGui::BeginTabBar ("Settings content")) {
			if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Settings"))) {
				if (ImGui::Button ("Re-Compute")) {
					m_GridHeight = m_TempGridHeight;
					OnAttachExtras ();
				}
				ImGui::InputInt ("Grid Height", &m_TempGridHeight);
				ImGui::Text ("Mouse At Square Number: %f", m_PixData);

				ImGui::EndTabItem ();
			}
			ImGui::EndTabBar ();
		}
	};
	virtual void OnUpdate (GLCore::Timestep ts) override;;
	virtual void OnEvent (GLCore::Event &event) override;
private:
	static const char *s_just_a_compute_shader;
	static const char *s_just_a_sqr_shader_vert;
	static const char *s_just_a_sqr_shader_frag;
	bool m_ComputeAgain = false;
	int m_TempGridHeight = 1;
	int m_GridHeight = 1;
	GLuint m_EquationsTexture = 0;
	uint32_t m_Eqn_TEX_Width = 40;
	
	GLuint m_OutIndexImg = 0;
	std::vector<Picture> pics;
	float m_PixData;
	glm::vec2 m_ViewPortSize;
	GLuint m_SubstituteTexture = 0;

	GLuint m_ImgIndexShaderID = 0;
	GLuint m_SqrIndexShaderID = 0;
	GLuint m_RelPosnShaderID = 0;
	GLuint m_ScaleShaderID= 0;
};