#pragma once
#include <cmath>
#include <GLcore.h>
#include <glm/gtc/matrix_transform.hpp>
#include <type_traits>
#include <limits>
#include "In-One-Weekend/base.h"
#include <GLCore/Util/Core/Framebuffer.h>

struct Transform_Buff // Note: no methods, pointers. This data will be directly send to GPU
{
	float Position[3];
	//float DeltaPosition[3]; // change in position since last frame
	float RotationMatrix[9];
	float Scale[3];
};
class Transform_Data
{
public:
	// Dynamic Implementation of calculation of AABB min, max
	virtual std::pair<glm::vec3, glm::vec3> CalculateBBMinMax () = 0;

	// If buffer needs to be updated of data change, turn updated = true
	virtual void OnImGuiRender () = 0;
	
	void OnImGuiRenderMin ()
	{
		Helper::IMGUI::DrawVec3Control ("Position", position);
		Helper::IMGUI::DrawVec3Control ("Rotation", rotation);
		Helper::IMGUI::DrawVec3Control ("Scale", scale);
	}


	// fills Transform Buffer provided, if any(or both) are inherited then handle extra data yourself
	bool FillTransformBuff (Transform_Buff *buffer)
	{
		buffer->Position[0] = position[0], buffer->Position[1] = position[1], buffer->Position[2] = position[2];

		glm::mat3 matrix = 
			Helper::MATH::MakeRotationZ (glm::radians (rotation.z)) // roll   <-can be pitch roll respectivly but who cares
			*Helper::MATH::MakeRotationX (glm::radians (rotation.x)) // pitch <-'
			*Helper::MATH::MakeRotationY (glm::radians (rotation.y)); // yaw
		for (uint32_t i = 0; i < 9; i++)
			buffer->RotationMatrix[i] = matrix[i/3][i%3];

		buffer->Scale[0] = scale[0], buffer->Scale[1] = scale[1], buffer->Scale[2] = scale[2];

		return false;
	}
public:
	bool updated = true; // 1st iteration
	glm::vec3 position;
	glm::vec3 last_position; // change in position since last frame
	glm::vec3 rotation; // in radians
	glm::vec3 scale;
//private: // To be calculated real-time
//	glm::vec3 bb_min;
//	glm::vec3 bb_max;
};

template<class Geometry_Data, class Geometry_Buff>
class RayTracing_Base : public ComputeAndSqrShader_Base
{
public:
	RayTracing_Base (const char *name, const char *discription = "just a base"
					 , const char *default_compute_shader_src = nullptr// = default_RT_compute_shader_src
	)
		: ComputeAndSqrShader_Base (name, discription, default_compute_shader_src)
	{}
	virtual ~RayTracing_Base () = default;

	void OnComputeShaderReloadBase ();

	void OnAttachBase ();
	void OnDetachBase ();
	void OnUpdateBase (GLCore::Timestep ts, void(*InvokeBeforeComputeShader)() = nullptr);
	void OnImGuiRenderBase ();

	virtual bool FillBuffer () = 0;
protected:     
	bool     m_UpdateSceneBVH = true; // Re-Update Ability to reuse buffer
	bool     m_RedrawFrame    = true;
	
	GLuint m_GeometryDataBufferTextureID = 0;

	/// (ImGui) Modifiable containers (High to Low Level)
	int m_TileSize[2] = { 1,1 };
	int m_NumberOfTilesPerFrame = 1; // Should always be greater 0, int for imgui
	
	int m_OutputResolutionHeight = 100; // width related to height

	// int m_numberOfSamplesPerPixel, for now as i don't have way to split it into nearest multiple of two no.s, i won't be attempting it instead use
	int m_numberOfSamplesPerPixelGridSize = 5; // will be directly updated in shader and recompile it
	int m_NumberOfBouncesPerRaySample = 3;

	int m_NumberOfGeometryToRender = 1;

	GLuint m_ComputeShaderOutputColorTextureID = 0;
	GLuint m_ComputeShaderOutputDepthTextureID = 0;

	static const int camera_MaxFocusPoints = 3;
	struct 
	{
		glm::vec3 Position;
		union
		{
			struct  
			{
				float pitch;   // in radians
				float yaw;	   // in radians
			};
			float pitch_yaw[2];// in radians
		};
		float FOV_y = glm::radians(60.0f);

		int NumberOfFocusPoints = 1; // max 3
		float FocusDist[3] = {10, 15, 20}; // max 3

		float ApertureSize = 1.0f; // in diameter

		glm::vec3 Front ()
		{
			glm::vec3 front;
			front.x = cos (glm::radians (yaw)) * cos (glm::radians (pitch));
			front.y = sin (glm::radians (pitch));
			front.z = sin (glm::radians (yaw)) * cos (glm::radians (pitch));
			return front;
		}
	}m_Camera;

	std::vector<Geometry_Data> m_GeometryData;
	std::vector<Geometry_Buff> m_GeometryBuffer;
private:
	/// Shader uniforms
	GLuint m_SceneHeirarchyBufferTextureID = 0;

	struct
	{ // Inside compute shader
		GLuint Position = 0;
		GLuint Direction = 0;
		GLuint FOV_y = 0; // float 
		GLuint NumOfFocusPts = 0; // int
		GLuint FocusDist[3] = { 0,0,0 }; // float, Distance from camera
		GLuint Aperture = 0; // float, Diameter of lens
	}m_CameraUniformID;

	GLuint m_MaxRayBouncesUniformID = 0;

	GLuint m_PixelOffsetUniformID = 0;


	////We don't need num of geometry as scene hierarchy will have it
	// GLuint m_NumOfGeometryUniformID = 0;

	/// Render/Shader Call related specification
	float    m_DeltatimeBetweenFrameDraw = 0.0f;

	int16_t  m_TileIndexs[2] = { 0,0 };
	uint16_t m_TileGridSize[2] = { 0,0 };
	int16_t  m_TileRingCorner00[2] = { 0,0 }; // minX, minY
	int16_t  m_TileRingCorner10[2] = { 0,0 }; // maxX, minY
	int16_t  m_TileRingCorner01[2] = { 0,0 }; // minX, maxY
	int16_t  m_TileRingCorner11[2] = { 0,0 }; // maxX, maxY
	int8_t  m_TileRingStep[2] = { 0,0 }; // maxX, maxY

	//static const char *default_RT_compute_shader_src;
};
//template<class Geometry_Data, class Geometry_Buff>
//__declspec(selectany) float RayTracing_Base<Geometry_Data, Geometry_Buff>::default_RT_compute_shader_src;
template<class Geometry_Data, class Geometry_Buff>
void RayTracing_Base<Geometry_Data, Geometry_Buff>::OnImGuiRenderBase ()
{
	auto Tooltip = [](const char *tip) {
		if (ImGui::IsItemHovered ()) {
			ImGui::BeginTooltip ();
			ImGui::PushTextWrapPos (ImGui::GetFontSize () * 35.0f);
			ImGui::TextUnformatted (tip);
			ImGui::PopTextWrapPos ();
			ImGui::EndTooltip ();
		}
	};

	if (ImGui::Button ("ReDraw Frame")) m_RedrawFrame = true;
	/// Render Specifications
	if (ImGui::InputInt ("Resolution Height", &m_OutputResolutionHeight)) {
		m_OutputResolutionHeight = MAX (100, MIN (m_OutputResolutionHeight, 1000)); // limit
		m_RedrawFrame = true;
	}
	Tooltip ("Set resolution of ray-traced output on screen.\nIncreasing increases load On GPU.");

	ImGui::InputInt2 ("Tile Size", &m_TileSize[0]);
	Tooltip ("Breaks image into regions called tiles, then renders tiles instead of whole Image per frame.\nIncreasing increases load On GPU.");

	ImGui::InputInt ("Output Tiles Per frame", &m_NumberOfTilesPerFrame);

	ImGui::InputInt ("No. of Samples Grid size", &m_numberOfSamplesPerPixelGridSize);
	Tooltip ("Set no. of samples per pixel grid size");

	ImGui::Text ("Total Samples: %d", m_numberOfSamplesPerPixelGridSize*m_numberOfSamplesPerPixelGridSize);

	ImGui::InputInt ("No.Of ray bounce", &m_NumberOfBouncesPerRaySample);
	Tooltip ("If you see artifacts in image, try decreasing this");

	if (ImGui::TreeNode ("Camera")) {
		ImGui::Indent ();
		m_RedrawFrame |= ImGui::DragFloat3 ("Position", &m_Camera.Position[0]);
		m_RedrawFrame |= ImGui::DragFloat2 ("Pitch & Yaw resp.", &m_Camera.pitch);

		float FOV_y_deg = glm::degrees (m_Camera.FOV_y);
		if (ImGui::DragFloat ("Field Of View (vertical, in degrees)", &FOV_y_deg)) {
			m_Camera.FOV_y = glm::radians (FOV_y_deg);
			m_RedrawFrame = true;
		}

		if (ImGui::InputInt ("No. Of Focus", &m_Camera.NumberOfFocusPoints)) {
			constexpr int temp = sizeof (m_Camera.FocusDist)/sizeof (m_Camera.FocusDist[0]);
			
			m_RedrawFrame = m_Camera.NumberOfFocusPoints >= 0 && m_Camera.NumberOfFocusPoints <= temp; // Changed
			// If its between range i.e 👍 else its invalid

			m_Camera.NumberOfFocusPoints = MAX (0, MIN (m_Camera.NumberOfFocusPoints, temp));
		}
		for (uint8_t i = 0; i < m_Camera.NumberOfFocusPoints; i++) {
			ImGui::PushID (i);
			
			//ImGui::Selectable ("select", &);
			//ImGui::SameLine ();

			std::string tmp = "Focus Dist #" + std::to_string (i + 1);

			m_RedrawFrame |= ImGui::DragFloat (tmp.c_str (), &m_Camera.FocusDist[i]);
			ImGui::PopID ();
		}

		ImGui::DragFloat ("Aperture Size", &m_Camera.ApertureSize, 0.01f, 0.0f, 5.0f);

		ImGui::Unindent ();

		ImGui::TreePop ();
	}

	if (ImGui::InputInt ("Number of objects in Scene", &m_NumberOfGeometryToRender)) {
		m_NumberOfGeometryToRender = MAX (0, MIN (m_NumberOfGeometryToRender, std::numeric_limits<uint16_t>::max ()));
		if (m_GeometryData.capacity () < m_NumberOfGeometryToRender)
			m_GeometryData.resize (m_NumberOfGeometryToRender), m_RedrawFrame = true;
		if (m_GeometryBuffer.capacity () < m_NumberOfGeometryToRender)
			m_GeometryBuffer.resize (m_NumberOfGeometryToRender), m_RedrawFrame = true;
	}
	if (ImGui::CollapsingHeader ("Objects:")) {
		ImGui::Indent ();

		for (uint16_t i = 0; i < m_NumberOfGeometryToRender; i++) {
			ImGui::PushID (i);
			std::string tmp = "Entity #" + std::to_string (i + 1);
			if (ImGui::TreeNode (tmp.c_str ())) {
				m_GeometryData[i].updated |= ImGui::DragFloat3 ("Position", &m_GeometryData[i].position[0]);
				m_GeometryData[i].updated |= ImGui::DragFloat3 ("Rotation", &m_GeometryData[i].rotation[0]);
				m_GeometryData[i].updated |= ImGui::DragFloat3 ("Scale", &m_GeometryData[i].scale[0]);
				m_GeometryData[i].OnImGuiRender ();

				m_RedrawFrame |= m_GeometryData[i].updated;
				ImGui::TreePop ();
			}
			ImGui::PopID ();
		}

		ImGui::Unindent ();
	}
}
template<class Geometry_Data, class Geometry_Buff>
void RayTracing_Base<Geometry_Data, Geometry_Buff>::OnUpdateBase (GLCore::Timestep ts, void(*InvokeBeforeComputeShader)())
{
	m_DeltatimeBetweenFrameDraw += ts;
	
	glClearColor (0.9f, 0.1f, 0.1f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (m_RedrawFrame)
	{
		{// Copy buffer
			bool resize_buff_tex = false, refill_buff_tex = false;
			if (m_GeometryData.size() < m_NumberOfGeometryToRender)
			{
				m_GeometryData.resize (m_NumberOfGeometryToRender);
				m_GeometryBuffer.resize (m_NumberOfGeometryToRender);
				resize_buff_tex = refill_buff_tex = true;
			}

			refill_buff_tex |= FillBuffer (); // Fill buffer

			if (m_GeometryDataBufferTextureID == 0) {
				m_GeometryDataBufferTextureID = Helper::TEXTURE_2D::Upload (nullptr, sizeof (Geometry_Buff)/sizeof (float[4]), m_GeometryBuffer.size (), GL_RGBA32F, GL_RGBA, GL_FLOAT);
				resize_buff_tex = false;
			}
			if (resize_buff_tex) {
				glActiveTexture (GL_TEXTURE0);
				glBindTexture (GL_TEXTURE_2D, m_GeometryDataBufferTextureID);
				// (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
				glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, sizeof (Geometry_Buff)/sizeof (float[4]), m_GeometryBuffer.size (), 0, GL_RGBA, GL_FLOAT, nullptr);
				
				resize_buff_tex = false;
			}

			if (refill_buff_tex)
			{
				Helper::TEXTURE_2D::SetData (m_GeometryDataBufferTextureID, sizeof (Geometry_Buff)/sizeof (float[4]), m_GeometryBuffer.size (), GL_RGBA, GL_FLOAT, m_GeometryBuffer.data (), 0);
			}   
		}

		m_DeltatimeBetweenFrameDraw = 0;
		m_RedrawFrame = false;

		float AspectRatio = ViewportSize ().x/ViewportSize ().y;
		m_TileGridSize[0] = int(m_OutputResolutionHeight*AspectRatio)/m_TileSize[0];
		m_TileGridSize[1] = m_OutputResolutionHeight/m_TileSize[1];
		m_TileIndexs[0] = (m_TileGridSize[0] - 0.1f)/2;
		m_TileIndexs[1] = (m_TileGridSize[1] - 0.1f)/2;

		m_TileRingCorner00[0] = m_TileIndexs[0];
		m_TileRingCorner00[1] = m_TileIndexs[1];
		m_TileRingCorner01[0] = m_TileIndexs[0];
		m_TileRingCorner01[1] = m_TileIndexs[1] + 1;
		m_TileRingCorner10[0] = m_TileIndexs[0] + 1;
		m_TileRingCorner10[1] = m_TileIndexs[1] - 1; // for next ring
		m_TileRingCorner11[0] = m_TileIndexs[0] + 1;
		m_TileRingCorner11[1] = m_TileIndexs[1] + 1;
		m_TileRingStep[0] = 0;
		m_TileRingStep[1] = 0;
	}
	for (uint16_t i = 0; i < m_NumberOfTilesPerFrame; i++) {
		bool drawableTile = false;
		while (!drawableTile) {
			if (MIN (m_TileIndexs[0], m_TileIndexs[1]) < MAX (m_TileGridSize[0], m_TileGridSize[1]) + 1) {
				if (m_TileIndexs[1] == m_TileRingCorner00[1] && m_TileIndexs[0] == m_TileRingCorner00[0]) {
					m_TileRingCorner00[0]--, m_TileRingCorner00[1]--;
					m_TileRingStep[0] = 0, m_TileRingStep[1] = 1;
				}
				if (m_TileIndexs[1] == m_TileRingCorner01[1] && m_TileIndexs[0] == m_TileRingCorner01[0]) {
					m_TileRingCorner01[0]--, m_TileRingCorner01[1]++;
					m_TileRingStep[0] = 1, m_TileRingStep[1] = 0;
				}
				if (m_TileIndexs[1] == m_TileRingCorner11[1] && m_TileIndexs[0] == m_TileRingCorner11[0]) {
					m_TileRingCorner11[0]++, m_TileRingCorner11[1]++;
					m_TileRingStep[0] = 0, m_TileRingStep[1] = -1;
				}
				if (m_TileIndexs[1] == m_TileRingCorner10[1] && m_TileIndexs[0] == m_TileRingCorner10[0]) {
					m_TileRingCorner10[0]++, m_TileRingCorner10[1]--;
					m_TileRingStep[0] = -1, m_TileRingStep[1] = 0;
				}
				drawableTile = m_TileIndexs[1] > -1 && m_TileIndexs[1] < m_TileGridSize[1]+1 && m_TileIndexs[0] > -1 && m_TileIndexs[0] < m_TileGridSize[0]+1;
				if (drawableTile) {
					glUseProgram (m_ComputeShaderProgID);

					glUniform2i (m_PixelOffsetUniformID, int (m_TileIndexs[0]*m_TileSize[0]), int (m_TileIndexs[1]*m_TileSize[1]));
					glUniform1i (m_MaxRayBouncesUniformID, m_NumberOfBouncesPerRaySample);

					glUniform3f (m_CameraUniformID.Position, m_Camera.Position[0], m_Camera.Position[1], m_Camera.Position[2]);
					glm::vec3 temp = m_Camera.Front ();
					glUniform3f (m_CameraUniformID.Direction, temp.x, temp.y, temp.z);
					glUniform1f (m_CameraUniformID.FOV_y, m_Camera.FOV_y);
					glUniform1i (m_CameraUniformID.NumOfFocusPts, m_Camera.NumberOfFocusPoints);
					glUniform1f (m_CameraUniformID.FocusDist[0], m_Camera.FocusDist[0]);
					glUniform1f (m_CameraUniformID.FocusDist[1], m_Camera.FocusDist[1]);
					glUniform1f (m_CameraUniformID.FocusDist[2], m_Camera.FocusDist[2]);
					glUniform1f (m_CameraUniformID.Aperture, m_Camera.ApertureSize);

					glActiveTexture (GL_TEXTURE0);
					glBindTexture (GL_TEXTURE_2D, m_SceneHeirarchyBufferTextureID);
					glActiveTexture (GL_TEXTURE1);
					glBindTexture (GL_TEXTURE_2D, m_GeometryDataBufferTextureID);

					if(InvokeBeforeComputeShader) InvokeBeforeComputeShader ();

					float AspectRatio = ViewportSize ().x/ViewportSize ().y;
					glDispatchCompute ((m_TileIndexs[0] >= m_TileGridSize[0] ? int(m_OutputResolutionHeight*AspectRatio) % m_TileSize[0] : m_TileSize[0])
									  ,(m_TileIndexs[1] >= m_TileGridSize[1] ? m_OutputResolutionHeight % m_TileSize[1] : m_TileSize[1]), 1);

				}
				m_TileIndexs[0] += m_TileRingStep[0], m_TileIndexs[1] += m_TileRingStep[1];
			}
		}
	}

	// make sure writing to image has finished before read
	glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
template<class Geometry_Data, class Geometry_Buff>
void RayTracing_Base<Geometry_Data, Geometry_Buff>::OnDetachBase ()
{
	DeleteQuadVAO ();

	if(m_ComputeShaderOutputColorTextureID) glDeleteTextures (1, &m_ComputeShaderOutputColorTextureID);
	if(m_ComputeShaderOutputDepthTextureID) glDeleteTextures (1, &m_ComputeShaderOutputDepthTextureID);
	m_ComputeShaderOutputColorTextureID = m_ComputeShaderOutputDepthTextureID = 0;
	
	DeleteComputeShader ();
	DeleteSquareShader ();
}
template<class Geometry_Data, class Geometry_Buff>
void RayTracing_Base<Geometry_Data, Geometry_Buff>::OnAttachBase ()
{
	static_assert(std::is_base_of<Transform_Data, Geometry_Data>::value);
	static_assert(std::is_base_of<Transform_Buff, Geometry_Buff>::value);
	static_assert(sizeof(Geometry_Buff) % sizeof(float[4]) == 0);

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ReloadComputeShader (); ReloadSquareShader ();
	ReGenQuadVAO ();

	float AspectRatio = ViewportSize ().x/ViewportSize ().y;
	if (m_ComputeShaderOutputColorTextureID == 0)
		m_ComputeShaderOutputColorTextureID = Helper::TEXTURE_2D::Upload (nullptr, uint32_t (m_OutputResolutionHeight*AspectRatio), uint32_t (m_OutputResolutionHeight), GL_RGBA32F, GL_RGBA, GL_FLOAT);
	if (m_ComputeShaderOutputDepthTextureID == 0)
		m_ComputeShaderOutputDepthTextureID = Helper::TEXTURE_2D::Upload (nullptr, uint32_t (m_OutputResolutionHeight*AspectRatio), uint32_t (m_OutputResolutionHeight), GL_R32F, GL_RED, GL_FLOAT);
	glBindImageTexture (0, m_ComputeShaderOutputColorTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA); // Color
	glBindImageTexture (1, m_ComputeShaderOutputDepthTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RED); // Depth

	if (m_GeometryBuffer.capacity () < m_NumberOfGeometryToRender)
		m_GeometryBuffer.resize (m_NumberOfGeometryToRender);
	if (m_GeometryData.capacity () < m_NumberOfGeometryToRender)
		m_GeometryData.resize (m_NumberOfGeometryToRender);
}
template<class Geometry_Data, class Geometry_Buff>
void RayTracing_Base<Geometry_Data, Geometry_Buff>::OnComputeShaderReloadBase ()
{
	m_CameraUniformID.Position  = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.Position");
	m_CameraUniformID.Direction = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.Direction");
	m_CameraUniformID.FOV_y     = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.FOV_y");
	m_CameraUniformID.FocusDist[0]  = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.FocusDist[0]");
	m_CameraUniformID.FocusDist[1]  = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.FocusDist[1]");
	m_CameraUniformID.FocusDist[2]  = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.FocusDist[2]");
	m_CameraUniformID.NumOfFocusPts = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.NumOfFocusPts");
	m_CameraUniformID.Aperture = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.Aperture");

	m_MaxRayBouncesUniformID = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfBounce");
	m_PixelOffsetUniformID  = glGetUniformLocation (m_ComputeShaderProgID, "u_ImgOffset");
}

