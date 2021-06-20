﻿#pragma once
#include <GLCore.h>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include "../base.h"

namespace In_One_Weekend
{
	class Adding_Materials: public ComputeAndSqrShader_Base
	{
	private:
		struct GeometryBuff{
			float position[3] = { 0,0,0 };
			float inv_rotn_mat3[3*3] = { 0,0,0,0,0,0,0,0,0 };
			float scale[3] = { 0,0,0 };
			float color[3] = { 0,0,0 };
		};
	public:
		Adding_Materials ()
			: ComputeAndSqrShader_Base ("InOneWeekend - 03_Materials", "just writing on Screen", Helper::ReadFileAsString("./Src/In-One-Weekend/03_Shadows_and_Materials/computeShaderSrc.glsl", '#').c_str ())
		{}
		~Adding_Materials () = default;
		virtual void OnDetach () override;
		virtual void OnAttach () override;
		virtual void OnUpdate (GLCore::Timestep ts);
		virtual void OnEvent (GLCore::Event &event) override;
		virtual void OnImGuiRender () override;
		virtual void ImGuiMenuOptions () override {};

		virtual void OnComputeShaderReload () override;
		virtual void OnSquareShaderReload () override;

		glm::vec3 FrontFromPitchYaw (float pitch, float yaw);

		enum Geom_type{
			None = 0,
			CUBOID = 1,
			ELLIPSOID = 2,
		};
		struct Geometry
		{
		public:
			Geometry () = default;
			// Returns whether anything changed in buffer
			bool FillBuffer (GeometryBuff &buffer) {
				bool changed = false;
				for (int i = 0; i < 3; i++) {
					if (buffer.position[i] != Position[i%3])
						buffer.position[i]  = Position[i%3], changed = true;
				}
				for (int i = 0; i < 9; i++) {
					if (buffer.inv_rotn_mat3[i] != _inv_rotation_matrix[i/3][i%3])
						buffer.inv_rotn_mat3[i]  = _inv_rotation_matrix[i/3][i%3], changed = true;
				}
				for (int i = 0; i < 3; i++) {
					if (buffer.scale[i] != Scale[i%3])
						buffer.scale[i]  = Scale[i%3], changed = true;
				}
				for (int i = 0; i < 3; i++) {
					if (buffer.color[i] != Color[i%3])
						buffer.color[i]  = Color[i%3], changed = true;
				}
				return changed;
			}
			void ResetInvRotationMatrix ()
			{
				_inv_rotation_matrix =
					glm::inverse (
						Helper::MATH::MakeRotationZ (glm::radians (Rotation.z)) // roll       <-can be pitch roll respectivly but who cares
							*Helper::MATH::MakeRotationX (glm::radians (Rotation.x)) // pitch <-'
								*Helper::MATH::MakeRotationY (glm::radians (Rotation.y)) // yaw
					);
				// Extra stuff like updating the buffer of data to be uploaded
			}
		public:
			// For ImGui
			Geom_type Typ = Geom_type::CUBOID;
			glm::vec3 Position = {0, 0, 0};
			glm::vec3 Rotation = {0, 0, 0};
			glm::vec3 Scale = {1, 1, 1};
			glm::vec3 Color = {1, 0, 0};
		private:
			glm::mat3 _inv_rotation_matrix = glm::mat3(1.0f);
		};

		// Copy Geometry data from Object to data buffer and upload as textures to GPU
		void CopyObjBuffer ();
	private:
		bool m_UpdateFrame = false;

		bool m_ShowNormals = false;
		bool m_Diffused = true;
		bool m_Cull_Frontside = false;
		bool m_Cull_Backside = true;

		int16_t m_TileIndexs[2] = {0, 0};
		uint16_t m_MaxTileIndexs[2] = {0, 0};
		int16_t m_TileRingLimit00[2] = { 0, 0 };
		int16_t m_TileRingLimit01[2] = { 0, 0 };
		int16_t m_TileRingLimit10[2] = { 0, 0 };
		int16_t m_TileRingLimit11[2] = { 0, 0 };
		int8_t m_TileIndexLastStep[2] = { 0, 0 };
		int m_NumberOfTilesAtATime = 1;

		GLuint m_ComputeShaderOutputTex = 0;
		GLuint m_GroupsDataBufferTex = 0;
		GLuint m_GroupsIDBufferTex = 0;

		GLuint m_FocusDistUniLoc = 0;
		GLuint m_CamPosnUniLoc = 0;
		GLuint m_CamDirnUniLoc = 0;
		GLuint m_ShowNormalsUniLoc = 0;
		GLuint m_NumOfGeometryUniLoc = 0;
		GLuint m_Cull_FrontsideUniLoc = 0;
		GLuint m_Cull_BacksideUniLoc = 0;
		GLuint m_NumOfBouncesUniLoc = 0;
		GLuint m_NumOfSamplesUniLoc = 0;
		GLuint m_DiffusedUniLoc = 0;
		GLuint m_TileIndexsLocation = 0; // to track which tile is being processed
		GLuint m_TileSizeLocation = 0;
		
		const int m_NumOfObjInGroup = 2;
		int m_NumOfBouncesPerRay = 10;
		int m_NumOfSamplesPerPixel = 100;

		float m_FocusDist = 1.0f;
		glm::vec3 m_CameraPosn = glm::vec3 (0, 4.2, 10);
		glm::vec2 m_CameraPitchYaw = glm::vec2 (-23, -90);

		const glm::ivec3 Work_Group_Count = { 0,0,0 }, Work_Group_Size = { 0,0,0 };
		int m_OutputResolutionHeight = 700;
		glm::ivec2 m_OutputTexDimensions = glm::ivec2 (700, 700);
		const glm::ivec2 m_TileSize = glm::ivec2 (100, 100);

		// each Element has vec3[6], bool is there to keep track whether buffer is ready needed to be re-uploaded
		std::pair<std::vector<GeometryBuff>, bool> m_GroupsDataBuffer;
		std::pair<std::vector<float>, bool> m_GroupsIDBuffer;
		std::vector<Geometry> m_GeometryGroup;
		//static const char *s_ComputeShader;
	};
};
