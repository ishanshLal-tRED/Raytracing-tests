#include "In-One-Weekend/base.h"
#include <GLCoreUtils.h>
#include <stack>
#include <thread>
#include <chrono>

#include "LBVH.h"
namespace In_Next_Week
{
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
		virtual std::pair<glm::vec3, glm::vec3> CalculateBBMinMax ()
		{
			glm::mat3 matrix =
				Helper::MATH::MakeRotationZ (glm::radians (rotation.z)) // roll   <-can be pitch roll respectivly but who cares
				*Helper::MATH::MakeRotationX (glm::radians (rotation.x)) // pitch <-'
				*Helper::MATH::MakeRotationY (glm::radians (rotation.y)); // yaw
			matrix = matrix*glm::mat3 (
				scale.x, 0, 0,
				0, scale.y, 0,
				0, 0, scale.z
			);
			float x = sqrt (matrix[0][0]*matrix[0][0] + matrix[1][0]*matrix[1][0] + matrix[2][0]*matrix[2][0]);
			float y = sqrt (matrix[0][1]*matrix[0][1] + matrix[1][1]*matrix[1][1] + matrix[2][1]*matrix[2][1]);
			float z = sqrt (matrix[0][2]*matrix[0][2] + matrix[1][2]*matrix[1][2] + matrix[2][2]*matrix[2][2]);

			return { glm::vec3 (-x, -y, -z) + position, glm::vec3 (x, y, z)  + position };
		}

		// If buffer needs to be updated of data change, turn updated = true
		virtual bool OnImGuiRender () = 0;

		inline bool OnImGuiRenderMin ()
		{
			return Helper::IMGUI::DrawVec3Control ("Position", position) 
				| Helper::IMGUI::DrawVec3Control ("Rotation", rotation) 
				| Helper::IMGUI::DrawVec3Control ("Scale", scale);

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
		glm::vec3 position = glm::vec3(3);
		glm::vec3 last_position = glm::vec3 (0); // change in position since last frame
		glm::vec3 rotation = glm::vec3(0); // in radians
		glm::vec3 scale = glm::vec3(1);
		//private: // To be calculated real-time
		//	glm::vec3 bb_min;
		//	glm::vec3 bb_max;
	};

	template<typename GeometryData, typename GeometryBuff>
	class RT_Base
		: public ComputeAndSqrShader_Base
	{
	public:
		RT_Base (const char *name, const char *discription = "RayTracing Base"
				 , const char *computeShaderSrc = ComputeAndSqrShader_Base::s_default_compute_shader
				 , const char *squareShaderFragSrc = ComputeAndSqrShader_Base::s_default_sqr_shader_frag // Maybe for post-processing
				 , const char *squareShaderVertSrc = ComputeAndSqrShader_Base::s_default_sqr_shader_vert)
			: ComputeAndSqrShader_Base (name, discription, computeShaderSrc, squareShaderVertSrc, squareShaderFragSrc)
		{ m_Camera.pitch = 0, m_Camera.yaw = 0;	}
		virtual ~RT_Base () = default;
	protected:
		void OnUpdateBase (GLCore::Timestep);
		void OnAttachBase ();
		void OnDetachBase ();
		void OnImGuiRenderBase ();
		
		void OnComputeShaderReloadBase ();
		virtual bool FillBuffer (GLCore::Timestep) = 0;
	protected:
		bool m_RedrawFrame = true;

		GLuint m_ComputeShaderOutputColorTextureID = 0;
		GLuint m_ComputeShaderOutputDepthTextureID = 0;
		
		GLuint m_GeometryBufferInputTextureID = 0;
		GLuint m_SceneHierarchyInputTextureID = 0;

		int m_RTOutputResolution = 100;
		float m_AspectRatio = 1.0f;
		float m_DeltatimeBetweenFrameDraw = 1.0f;
		
		int m_NumberOfGeometriesToRender = 3;
		int m_NumberOfSamplesPerPixel = 1;
		struct  
		{
			glm::vec3 Position = glm::vec3(0.0f);
			union
			{
				struct
				{
					float pitch;   // in radians
					float yaw;	   // in radians
				};
				float pitch_yaw[2];// in radians
			};
			float FOV_y = glm::radians (60.0f);
			
			glm::vec3 Front ()
			{
				glm::vec3 front;
				front.x = cos (glm::radians (yaw)) * cos (glm::radians (pitch));
				front.y = sin (glm::radians (pitch));
				front.z = sin (glm::radians (yaw)) * cos (glm::radians (pitch));
				return front;
			}
		}m_Camera;

		struct  
		{
			GLuint NumberOfGeometriesToRender = 0;

			struct  
			{
				GLuint Position;
				GLuint Direction;
				GLuint FOV_y;
			}Camera;
		}m_UniformID;

		std::vector<GeometryData> m_GeometryData;
		std::vector<GeometryBuff> m_GeometryBuff;

	};

	//################################

	template<typename GeometryData, typename GeometryBuff>
	void In_Next_Week::RT_Base<GeometryData, GeometryBuff>::OnImGuiRenderBase ()
	{
		m_RedrawFrame |= ImGui::Button ("ReDraw Frame");
		if (ImGui::DragInt ("Output Resolution", &m_RTOutputResolution)) {
			if (0 < m_RTOutputResolution && m_RTOutputResolution < 1001) {
				m_RedrawFrame = true;

				// Backup GL state
				GLenum last_active_texture; glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint *)&last_active_texture);
				GLuint last_texture; glGetIntegerv (GL_TEXTURE_BINDING_2D, (GLint *)&last_texture);

				// glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
				glActiveTexture (GL_TEXTURE0);
				glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputColorTextureID);
				glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, uint32_t (m_RTOutputResolution*m_AspectRatio), uint32_t (m_RTOutputResolution), 0, GL_RGBA, GL_FLOAT, nullptr);
				glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputDepthTextureID);
				glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, uint32_t (m_RTOutputResolution*m_AspectRatio), uint32_t (m_RTOutputResolution), 0, GL_RGBA, GL_FLOAT, nullptr);

				glActiveTexture (last_active_texture);
				glBindTexture (GL_TEXTURE_2D, last_texture);
			}
			m_RTOutputResolution = MAX (1, MIN (m_RTOutputResolution, 1000));
		}
		if (ImGui::InputInt ("Number Of Samples Per Pixel", &m_NumberOfSamplesPerPixel)) 
		{
			m_NumberOfSamplesPerPixel = MAX (m_NumberOfSamplesPerPixel, 1);
			constexpr uint32_t max_axis = 2;
			uint32_t WorkGrpSize[max_axis];
			{
				WorkGrpSize[0] = m_NumberOfSamplesPerPixel;
				WorkGrpSize[1] = 1;
			}
			uint32_t i[max_axis], j[max_axis], axis = 0; i[0] = 0, j[0] = 0;
			const char *ShaderSrc = m_ComputeShaderTXT.data ();
			bool resize = false;
			
			while (ShaderSrc[i[axis]] != '(')
				i[axis]++;
			i[axis] += 2; // '( local...'
			while (axis < max_axis) // ) of local_size_x = __, local_size_y = __)
			{				
				uint32_t temp = i[axis] - 1;
				while (ShaderSrc[temp] != 'l')
					temp++;
				
				static const char *campare[3] = { "local_size_x", "local_size_y", "local_size_z"};
				uint32_t k, campare_size = strlen(campare[axis]);
				for (k = 0; k < campare_size; k++) {
					if (ShaderSrc[temp + k] != campare[axis][k]) {
						temp++;
						break;
					}
				}
				if (k == campare_size) // success
				{
					resize |= i[axis] != temp;
					temp += k;
					k = 0;
					while (ShaderSrc[temp + k] != ',' && ShaderSrc[temp + k] != ')') // " = 0000"
						k++;
					resize |= k != 7; // condn, greater 7 or less 7

					if(!resize) {
						char *shader_src = m_ComputeShaderTXT.raw_data ();
						shader_src[temp + 0] = ' ';
						shader_src[temp + 1] = '=';
						shader_src[temp + 2] = ' ';

						bool Insert = false;
						uint8_t val;
						val = (WorkGrpSize[axis]/1000)%10; Insert |= val > 0; shader_src[temp + 3] = Insert ? '0' + val : ' ';
						val = (WorkGrpSize[axis]/100)%10;  Insert |= val > 0; shader_src[temp + 4] = Insert ? '0' + val : ' ';
						val = (WorkGrpSize[axis]/10)%10;   Insert |= val > 0; shader_src[temp + 5] = Insert ? '0' + val : ' ';
						val = (WorkGrpSize[axis]/1)%10;    Insert |= val > 0; shader_src[temp + 6] = Insert ? '0' + val : ' ';

					}
					j[axis] = temp + k;
					axis++;
					if(axis < max_axis) 
						i[axis] = j[axis - 1] + 2;
				}
			}
			if (resize) {
				const int block_width_change = (j[max_axis - 1] - (i[0] + strlen ("local_size__ = 0000") + (max_axis-1)*strlen (", local_size__ = 0000")));
				const uint32_t ShaderSrcTXT_size = strlen (ShaderSrc);
				char *shader_src = m_ComputeShaderTXT.raw_data ();
				
				if (block_width_change != 0) {
					if (block_width_change < 0) { // Less than necessary, shift outwards
						m_ComputeShaderTXT.resize (ShaderSrcTXT_size - block_width_change);
						for (uint32_t here = ShaderSrcTXT_size; here >= j[max_axis - 1]; here--)
							shader_src[here - block_width_change] = ShaderSrc[here];
					}
					else {// ELSE: More than necessary, shift inwards
						for (uint32_t here = 0; here < block_width_change; here++)
							shader_src[j[max_axis - 1] - here] = ' ';
						shader_src[j[max_axis - 1] - block_width_change] = ')';
					}
				}
				// ELSE: no resize only re-arrange
				j[max_axis - 1] -= block_width_change;
				const uint32_t size = j[max_axis - 1] - i[0];
				axis = 0;
				for (uint32_t here = 0, from = 0; here < size; here++, from++) {
					if (here < + (axis + 1)*strlen (" local_size__ = 0000")) {
						shader_src[here + i[0] - 1] =
							axis == 0 ? (" local_size_x = ____")[from] :
							axis == 1 ? (" local_size_y = ____")[from] :
							axis == 2 ? (" local_size_z = ____")[from] : ' ';
					} else {
						LOG_ASSERT (here == (axis + 1)*strlen (" local_size__ = 0000"));
						shader_src[here + i[0] - 1] = axis < max_axis - 1 ? ',' : ')';
						bool Insert = false;
						uint8_t val;
						val = (WorkGrpSize[axis]/1000)%10; Insert |= val > 0; shader_src[here + i[0] - 5] = Insert ? '0' + val : ' ';
						val = (WorkGrpSize[axis]/100)%10;  Insert |= val > 0; shader_src[here + i[0] - 4] = Insert ? '0' + val : ' ';
						val = (WorkGrpSize[axis]/10)%10;   Insert |= val > 0; shader_src[here + i[0] - 3] = Insert ? '0' + val : ' ';
						val = (WorkGrpSize[axis]/1)%10;    Insert |= val > 0; shader_src[here + i[0] - 2] = Insert ? '0' + val : ' ';
						
						axis++;
						from = -1;
					}
				}
				{
					uint32_t here = j[max_axis - 1];
					shader_src[here] = ')';
					bool Insert = false;
					uint8_t val;
					val = (WorkGrpSize[axis]/1000)%10; Insert |= val > 0; shader_src[here - 4] = Insert ? '0' + val : ' ';
					val = (WorkGrpSize[axis]/100)%10;  Insert |= val > 0; shader_src[here - 3] = Insert ? '0' + val : ' ';
					val = (WorkGrpSize[axis]/10)%10;   Insert |= val > 0; shader_src[here - 2] = Insert ? '0' + val : ' ';
					val = (WorkGrpSize[axis]/1)%10;    Insert |= val > 0; shader_src[here - 1] = Insert ? '0' + val : ' ';
					shader_src[here - 5] = ' ';
				}
			}
			ReloadComputeShader ();
		}
		if (ImGui::CollapsingHeader ("Camera", nullptr, ImGuiTreeNodeFlags_DefaultOpen  | ImGuiTreeNodeFlags_SpanAvailWidth)) {
			ImGui::Indent ();
			m_RedrawFrame |= Helper::IMGUI::DrawVec3Control ("Position", m_Camera.Position);
			m_RedrawFrame |= ImGui::DragFloat2 ("Pitch & Yaw resp.", &m_Camera.pitch);

			float FOV_y_deg = glm::degrees (m_Camera.FOV_y);
			if (ImGui::DragFloat ("Field Of View (vertical, in degrees)", &FOV_y_deg)) {
				m_Camera.FOV_y = glm::radians (FOV_y_deg);
				m_RedrawFrame = true;
			}
			ImGui::Separator ();
			ImGui::Separator ();
			ImGui::Unindent ();
		}

		if (ImGui::InputInt ("Number Of Geometries To Render", &m_NumberOfGeometriesToRender)) {
			m_RedrawFrame = true;
			m_NumberOfGeometriesToRender = MAX (1, m_NumberOfGeometriesToRender);
		}
		if (ImGui::CollapsingHeader ("Objects:")) {
			ImGui::Indent ();

			for (uint16_t i = 0; i < m_NumberOfGeometriesToRender; i++) {
				ImGui::PushID (i);
				std::string tmp = "Entity #" + std::to_string (i + 1);
				if (ImGui::TreeNode (tmp.c_str ())) {

					m_RedrawFrame |= m_GeometryData[i].updated |= m_GeometryData[i].OnImGuiRender ();;
					
					ImGui::TreePop ();
				}
				ImGui::PopID ();
			}

			ImGui::Unindent ();
		}
		if (ImGui::Button ("Print Hierarchy tree")) {
			auto [nodeBuff, buffSize] = LBVH::ConstructLBVH_Buff<GeometryData> (m_GeometryData.data (), m_NumberOfGeometriesToRender);
			std::stack<std::pair<LBVH::BVHNodeBuff *, uint32_t>> node_stack;
			node_stack.push ({ &nodeBuff[0], 0 });
			LOG_TRACE ("  TREE :");
			while (!node_stack.empty ())
			{
				auto [curr, lvl] = node_stack.top ();
				node_stack.pop ();
				for (uint32_t i = 0; i < lvl; i++)
					std::cout << "   ";
				std::cout << "BB [{" << curr->bb_min[0] << ", " << curr->bb_min[1] << ", " << curr->bb_min[2] << "} {" << curr->bb_max[0] << ", " << curr->bb_max[1] << ", " << curr->bb_max[2] << "}]";
				int left = int (curr->leftData + 0.1);
				int right = int (curr->leftData + 1.1);
				if (left < 0)
					std::cout << " obj: " << -curr->leftData;
				std::cout << "\n";
				if (left) node_stack.push ({ &nodeBuff[left], lvl+1 });
				if (right) node_stack.push ({ &nodeBuff[right], lvl+1 });
			}
			std::cout << "\n";
			delete[] nodeBuff;
		}
	}
	template<typename GeometryData, typename GeometryBuff>
	void In_Next_Week::RT_Base<GeometryData, GeometryBuff>::OnDetachBase ()
	{
		DeleteQuadVAO ();

		if (m_ComputeShaderOutputColorTextureID) glDeleteTextures (1, &m_ComputeShaderOutputColorTextureID);
		if (m_ComputeShaderOutputDepthTextureID) glDeleteTextures (1, &m_ComputeShaderOutputDepthTextureID);
		
		if (m_GeometryBufferInputTextureID) glDeleteTextures (1, &m_GeometryBufferInputTextureID);
		if (m_SceneHierarchyInputTextureID) glDeleteTextures (1, &m_SceneHierarchyInputTextureID);
		m_ComputeShaderOutputColorTextureID = m_ComputeShaderOutputDepthTextureID = m_GeometryBufferInputTextureID = m_SceneHierarchyInputTextureID = 0;

		DeleteComputeShader ();
		DeleteSquareShader ();
	}
	template<typename GeometryData, typename GeometryBuff>
	void In_Next_Week::RT_Base<GeometryData, GeometryBuff>::OnAttachBase ()
	{
		GLCore::Utils::EnableGLDebugging ();

		static_assert(std::is_base_of<Transform_Data, GeometryData>::value);
		static_assert(std::is_base_of<Transform_Buff, GeometryBuff>::value);
		static_assert(sizeof (GeometryBuff) % sizeof (float[4]) == 0);
		
		static_assert(sizeof (LBVH::BVHNodeBuff) % sizeof (float[4]) == 0); // CAUTION, forced padding

		glEnable (GL_DEPTH_TEST);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		ReloadComputeShader (); ReloadSquareShader ();
		ReGenQuadVAO ();

		if (m_ComputeShaderOutputColorTextureID == 0)
			m_ComputeShaderOutputColorTextureID = Helper::TEXTURE_2D::Upload (nullptr, uint32_t (m_RTOutputResolution*m_AspectRatio), uint32_t (m_RTOutputResolution), GL_RGBA32F, GL_RGBA, GL_FLOAT);
		if (m_ComputeShaderOutputDepthTextureID == 0)
			m_ComputeShaderOutputDepthTextureID = Helper::TEXTURE_2D::Upload (nullptr, uint32_t (m_RTOutputResolution*m_AspectRatio), uint32_t (m_RTOutputResolution), GL_R32F, GL_RED, GL_FLOAT);
		
		glBindImageTexture (0, m_ComputeShaderOutputDepthTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F); // Depth
		glBindImageTexture (1, m_ComputeShaderOutputColorTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); // Color

		if (m_GeometryBufferInputTextureID == 0) {
			m_GeometryBufferInputTextureID = Helper::TEXTURE_2D::Upload (nullptr, sizeof (GeometryBuff)/sizeof (float[4]), m_GeometryBuff.size (), GL_RGBA32F, GL_RGBA, GL_FLOAT);
		}
		if (m_SceneHierarchyInputTextureID == 0) { // CAUTION, data width
			m_SceneHierarchyInputTextureID = Helper::TEXTURE_2D::Upload (nullptr, sizeof (LBVH::BVHNodeBuff)/sizeof (float[4]), m_GeometryBuff.size () > 0 ? m_GeometryBuff.size ()*2 - 1 : 0, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		}
	}
	template<typename GeometryData, typename GeometryBuff>
	void In_Next_Week::RT_Base<GeometryData, GeometryBuff>::OnUpdateBase (GLCore::Timestep ts)
	{
		// GLuint drawFBOID, readFBOID;
		// glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING, &drawFBOID);
		// glGetIntegerv (GL_READ_FRAMEBUFFER_BINDING, &readFBOID);

		m_DeltatimeBetweenFrameDraw += ts;
		glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (m_RedrawFrame){ // launch compute shaders!
			m_RedrawFrame = false;
			{// Copy Buffer
				bool resize_buff_tex = false, refill_buff_tex = false;
				if (m_GeometryData.size () < m_NumberOfGeometriesToRender) {
					m_GeometryData.resize (m_NumberOfGeometriesToRender);
					m_GeometryBuff.resize (m_NumberOfGeometriesToRender);
					resize_buff_tex = refill_buff_tex = true;
				}
				
				refill_buff_tex |= FillBuffer (m_DeltatimeBetweenFrameDraw); // Fill_Buffer

				if (resize_buff_tex) {
					glActiveTexture (GL_TEXTURE0);
					glBindTexture (GL_TEXTURE_2D, m_GeometryBufferInputTextureID);
					// (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
					glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, sizeof (GeometryBuff)/sizeof (float[4]), m_GeometryBuff.size (), 0, GL_RGBA, GL_FLOAT, nullptr);

					glBindTexture (GL_TEXTURE_2D, m_SceneHierarchyInputTextureID);
					// (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
					glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, sizeof (LBVH::BVHNodeBuff)/sizeof (float[4]), m_GeometryBuff.size ()*2 - 1, 0, GL_RGBA, GL_FLOAT, nullptr);

					resize_buff_tex = false;
				}

				if (refill_buff_tex) {
					Helper::TEXTURE_2D::SetData (m_GeometryBufferInputTextureID, sizeof (GeometryBuff)/sizeof (float[4]), m_GeometryBuff.size (), GL_RGBA, GL_FLOAT, m_GeometryBuff.data (), 0);

					//std::this_thread::sleep_for (std::chrono::seconds (1));
					auto [nodeBuff, buffSize] = LBVH::ConstructLBVH_Buff<GeometryData> (m_GeometryData.data (), m_NumberOfGeometriesToRender); 
					LOG_ASSERT ((m_GeometryBuff.size ()*2 - 1) == buffSize);
					Helper::TEXTURE_2D::SetData (m_SceneHierarchyInputTextureID, sizeof (LBVH::BVHNodeBuff)/sizeof (float[4]), m_GeometryBuff.size ()*2 - 1, GL_RGBA, GL_FLOAT, nodeBuff, 0);
					delete[] nodeBuff;
				}
			}
			m_DeltatimeBetweenFrameDraw = 0;


			glUseProgram (m_ComputeShaderProgID);

			glUniform1i (m_UniformID.NumberOfGeometriesToRender, m_NumberOfGeometriesToRender);

			glUniform3f (m_UniformID.Camera.Position, m_Camera.Position.x, m_Camera.Position.y, m_Camera.Position.z);
			glm::vec3 front = m_Camera.Front ();
			glUniform3f (m_UniformID.Camera.Direction, front.x, front.y, front.z);
			glUniform1f (m_UniformID.Camera.FOV_y, m_Camera.FOV_y);

			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_GeometryBufferInputTextureID);
			glActiveTexture (GL_TEXTURE1);
			glBindTexture (GL_TEXTURE_2D, m_SceneHierarchyInputTextureID);

			glDispatchCompute (uint32_t (m_RTOutputResolution*m_AspectRatio), uint32_t (m_RTOutputResolution), 1);
			
			// make sure writing to image has finished before read
			glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}
	template<typename GeometryData, typename GeometryBuff>
	void In_Next_Week::RT_Base<GeometryData, GeometryBuff>::OnComputeShaderReloadBase ()
	{
		m_UniformID.NumberOfGeometriesToRender = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfGeometry");
		m_UniformID.Camera.Position = glGetUniformLocation (m_ComputeShaderProgID,  "u_Camera.Position");
		m_UniformID.Camera.Direction = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.Direction");
		m_UniformID.Camera.FOV_y = glGetUniformLocation (m_ComputeShaderProgID, "u_Camera.FOV_y");
		m_RedrawFrame = true;
	}
}