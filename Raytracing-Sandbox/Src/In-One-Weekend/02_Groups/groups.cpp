#include "groups.h"
#include <GLCoreUtils.h>
#include <GLCore/Core/KeyCodes.h>
namespace In_One_Weekend
{
//	const char *Groups::s_ComputeShader = R"(
//#version 440 core
//layout (local_size_x = 1, local_size_y = 1) in;
//layout (rgba32f, binding = 0) uniform image2D img_output;
//
//uniform float u_FocusDist;
//uniform vec3 u_CameraPosn;
//uniform vec3 u_CameraDirn;
//
//uniform int u_NumOfObj;
//layout (r32f, binding = 0) uniform sampler2D u_ObjGroupIDTexture;
//layout (rgba32f, binding = 1) uniform sampler2D u_ObjGroupDataTexture;
//
//
//const int CUBOID = 1;
//const int ELLIPSOID = 2;
////const int NumOfObj = textureSize(u_ObjGroupIDTexture, 0).x;
//struct Ray
//{
//	vec3 Orig;
//	vec3 Dirn;
//};
//
//uniform bool u_ShowNormal;
//
//float DistanceFrom(Ray ray, vec3 pt){
//	return length(cross(pt - ray.Orig, ray.Dirn));
//};
//Ray CreateRay (vec3 pt1, vec3 pt2)
//{
//	return Ray (pt1, normalize (pt2 - pt1));
//};
//vec3 BackgroundColor (Ray ray)
//{
//	float t = (ray.Dirn.y + 1.0)*0.5;
//	return ((1.0 - t)*vec3 (1.0, 1.0, 1.0) + t*vec3 (0.3, 0.4, 1.0));
//}
//float t_RayXObj(Ray ray, int Obj_typ, vec3 scale){
//	float t = -1.0;
//	switch(Obj_typ){
//		case ELLIPSOID:{
//				vec3 _xyz2,_xyz3;
//				_xyz2 = vec3(ray.Orig.x/scale.x, ray.Orig.y/scale.y, ray.Orig.z/scale.z);
//				_xyz3 = vec3(ray.Dirn.x/scale.x, ray.Dirn.y/scale.y, ray.Dirn.z/scale.z);
//				
//				float half_b = dot(_xyz2,_xyz3);
//				float a = dot(_xyz3,_xyz3);
//				float c = dot(_xyz2,_xyz2) - 1;
//				float determinant = half_b*half_b - a*c;
//				if(determinant > 0){
//					t = (-half_b - sqrt(determinant)) / a;
//				}
//			}; break;
//		case CUBOID:{
//			vec3 b_min = -scale*0.5f;
//			vec3 b_max = +scale*0.5f;
//			float t1 = (b_min[0] - ray.Orig[0])*(1.0/ray.Dirn[0]);
//		    float t2 = (b_max[0] - ray.Orig[0])*(1.0/ray.Dirn[0]);
//		
//		    float tmin = min(t1, t2);
//		    float tmax = max(t1, t2);
//		
//		    for (int i = 1; i < 3; ++i) {
//		        t1 = (b_min[i] - ray.Orig[i])*(1.0/ray.Dirn[i]);
//				t2 = (b_max[i] - ray.Orig[i])*(1.0/ray.Dirn[i]);
//		        
//				tmin = max(tmin, min(min(t1, t2), tmax));
//		        tmax = min(tmax, max(max(t1, t2), tmin));
//		    }
//		
//		    t = tmax > max(tmin, 0.0) ? tmin : -1;
//		}; break;
//	}
//	return (t > 0 ? t : -1);
//}
//vec3 SurfaceNormal(float t, Ray ray, int Obj_typ, vec3 scale){
//	vec3 HitPoint = ray.Orig + t*ray.Dirn;
//	switch(Obj_typ){
//		case ELLIPSOID:
//			return vec3(HitPoint.x/scale.x*scale.x, HitPoint.y/scale.y*scale.y, HitPoint.z/scale.z*scale.z);
//		case CUBOID:
//			float min_dist = abs(HitPoint[0] - scale[0]*0.5);
//			int index = 0;
//
//			float dist = abs(HitPoint[0] + scale[0]*0.5);
//			if(min_dist > dist)
//				min_dist = dist, index = 1;
//			
//			for (int i = 1; i < 3; ++i) {
//				dist = abs(HitPoint[i] - scale[i]*0.5);
//				if(min_dist > dist)
//					min_dist = dist, index = i*2;
//				dist = abs(HitPoint[i] + scale[i]*0.5);
//				if(min_dist > dist)
//					min_dist = dist, index = i*2 + 1;
//		    };
//			vec3 result = vec3(0, 0, 0);
//			result[index/2] = index%2 == 0 ? 1 : -1;
//			// result = vec3(t/20, t/15, t/10); // for some beautiful result
//			return result;
//	}
//	return vec3(0,0,0);
//}
//
//vec4 out_Pixel (ivec2 pixel_coords)
//{
//	ivec2 img_size = imageSize (img_output);
//	float aspectRatio = float (img_size.x)/img_size.y;
//	vec3 world_up = vec3 (0, 1, 0);
//	vec3 cam_right = cross (u_CameraDirn, world_up);
//	vec3 cam_up = cross (cam_right, u_CameraDirn);
//	float scr_x = (pixel_coords.x*2.0 - img_size.x)/(2.0*img_size.x);
//	scr_x *= aspectRatio;
//	float scr_y = (pixel_coords.y*2.0 - img_size.y)/(2.0*img_size.y);
//
//	vec3 point_in_scr_space = u_CameraPosn + u_CameraDirn*u_FocusDist + cam_right*scr_x + cam_up*scr_y;
//	
//// Ray and other stuff
//	Ray cam_ray = CreateRay (u_CameraPosn, point_in_scr_space);
//	
//// Color
//	vec3 color = BackgroundColor(cam_ray); // background
//	
////Depth Testing
//	vec3 position, scale, obj_color;
//	mat3 matrix;
//	int Type;
//	float min_t_depth = 32000; // FLT_MAX
//	const int id_tex_size_x = textureSize(u_ObjGroupIDTexture, 0).x;
//	const ivec2 data_tex_size = textureSize(u_ObjGroupDataTexture, 0);
//	for(int i = 0; i < u_NumOfObj; i++){
//		Type = int(texture(u_ObjGroupIDTexture, vec2((i+0.1)/id_tex_size_x, 0.1)).r);
//		position  = texture(u_ObjGroupDataTexture, vec2(0.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;
//		matrix[0] = texture(u_ObjGroupDataTexture, vec2(1.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;
//		matrix[1] = texture(u_ObjGroupDataTexture, vec2(2.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;
//		matrix[2] = texture(u_ObjGroupDataTexture, vec2(3.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;
//		scale     = texture(u_ObjGroupDataTexture, vec2(4.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;
//		obj_color = texture(u_ObjGroupDataTexture, vec2(5.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;
//
//		vec3 transformed_orig = matrix*(u_CameraPosn - position);
//		vec3 transformed_point_in_scr = matrix*(point_in_scr_space - position);
//		Ray transf_cam_ray = CreateRay (transformed_orig, transformed_point_in_scr);
//
//		float t = t_RayXObj(transf_cam_ray, Type, scale);
//		if(min_t_depth > t && t > 0){
//			if(u_ShowNormal)
//				color = SurfaceNormal(t, transf_cam_ray, Type, scale);
//			else
//				color = obj_color;
//			min_t_depth = t;
//		}
//	}
//
//	return vec4 (color, 1.0);
//}
//void main ()
//{
//	// get index in global work group i.e x,y position
//	ivec2 pixel_coords = ivec2 (gl_GlobalInvocationID.xy);
//
//	// base pixel color for image
//	vec4 pixel = out_Pixel (pixel_coords);
//
//	// output to a specific pixel in the image
//	imageStore (img_output, pixel_coords, pixel);
//}
//)";
	void Groups::OnAttach ()
	{
		GLCore::Utils::EnableGLDebugging ();

		glEnable (GL_DEPTH_TEST);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		ReloadComputeShader (); ReloadSquareShader ();
		ReGenQuadVAO ();
		{
			int *work_grp_cnt = (int *)((void *)(&Work_Group_Count[0]));
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

			int *work_grp_size = (int *)((void *)(&Work_Group_Size[0]));
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
			glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
		}
		if (!m_ComputeShaderOutputTex) {
			// dimensions of the image
			int tex_w = m_OutputTexDimensions.x, tex_h = m_OutputTexDimensions.y;

			m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, tex_w, tex_h, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR);

			// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
			glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		}

		if (m_GroupsIDBuffer.first.capacity () < m_NumOfObjInGroup) {
			m_GroupsIDBuffer.first.resize (m_NumOfObjInGroup);
		}
		if (m_GroupsDataBuffer.first.capacity () < m_NumOfObjInGroup) {
			m_GroupsDataBuffer.first.resize (m_NumOfObjInGroup);
		}
		while (m_GeometryGroup.size () < m_NumOfObjInGroup) {
			m_GeometryGroup.emplace_back (Geometry ());
		}
	}
	void Groups::OnDetach ()
	{
		DeleteQuadVAO ();

		glDeleteTextures (1, &m_ComputeShaderOutputTex);
		m_ComputeShaderOutputTex = 0;

		DeleteComputeShader ();
		DeleteSquareShader ();
	}
	void Groups::OnUpdate (GLCore::Timestep ts)
	{
		CopyObjBuffer ();

		glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		{ // launch compute shaders!
			glUseProgram (m_ComputeShaderProgID);

			glUniform1i (m_ShowNormalsUniLoc, int (m_ShowNormals));
			glUniform1i (m_NumOfGeometryUniLoc, m_NumOfObjInGroup);

			glUniform1f (m_FocusDistUniLoc, m_FocusDist);
			glm::vec3 cam_dirn = FrontFromPitchYaw (m_CameraPitchYaw.x, m_CameraPitchYaw.y);
			glUniform3f (m_CamDirnUniLoc, cam_dirn.x, cam_dirn.y, cam_dirn.z);
			glUniform3f (m_CamPosnUniLoc, m_CameraPosn.x, m_CameraPosn.y, m_CameraPosn.z);

			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_GroupsIDBufferTex);
			glActiveTexture (GL_TEXTURE1);
			glBindTexture (GL_TEXTURE_2D, m_GroupsDataBufferTex);

			glDispatchCompute (m_OutputTexDimensions.x, m_OutputTexDimensions.y, 1);
		}

		// make sure writing to image has finished before read
		glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		{ // normal drawing pass
			glClear (GL_COLOR_BUFFER_BIT);
			glUseProgram (m_SquareShaderProgID);
			glBindVertexArray (m_QuadVA);
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, m_ComputeShaderOutputTex);
			glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		}
	}
	void Groups::OnEvent (GLCore::Event &event)
	{
		GLCore::EventDispatcher dispatcher (event);
		dispatcher.Dispatch<GLCore::LayerViewportResizeEvent> (
			[&](GLCore::LayerViewportResizeEvent &e) {

				m_OutputTexDimensions.x = (float (e.GetWidth ())/e.GetHeight ())*m_OutputTexDimensions.y;

				glDeleteTextures (1, &m_ComputeShaderOutputTex);
				m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, m_OutputTexDimensions.x, m_OutputTexDimensions.y, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR);

				// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
				glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

				return false;
			});
		dispatcher.Dispatch<GLCore::KeyPressedEvent> (
			[&](GLCore::KeyPressedEvent &e) {
				float &pitch = m_CameraPitchYaw.x;
				float &yaw = m_CameraPitchYaw.y;
				glm::vec3 front = FrontFromPitchYaw (pitch, yaw), right, up;

				static constexpr glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
				right = glm::normalize (glm::cross (front, worldUp));
				// up = glm::normalize (glm::cross (right, front));

				switch (e.GetKeyCode ()) {
					case GLCore::Key::Up:
						pitch = MIN (pitch + 0.3f, 89.0f); break;
					case GLCore::Key::Down:
						pitch = MAX (pitch - 0.3f, -89.0f); break;
					case GLCore::Key::Left:
						yaw = MOD (yaw - 0.3f, 360.0f); break;
					case GLCore::Key::Right:
						yaw = MOD (yaw + 0.3f, 360.0f); break;
					case GLCore::Key::W:
						m_CameraPosn += front*0.1f; break;
					case GLCore::Key::S:
						m_CameraPosn -= front*0.1f; break;
					case GLCore::Key::A:
						m_CameraPosn -= right*0.1f; break;
					case GLCore::Key::D:
						m_CameraPosn += right*0.1f; break;
				}

				return false;
			});
	}
	void Groups::OnImGuiRender ()
	{
		using namespace GLCore;
		ImGui::Begin (ImGuiLayer::UniqueName ("Just a window"));
		if (ImGui::BeginTabBar (ImGuiLayer::UniqueName ("Shaders Content"))) {
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Settings"))) {

				ImGui::Text ("Max Compute Work Group\n Count: %d, %d, %d\n Size:  %d, %d, %d", Work_Group_Count[0], Work_Group_Count[1], Work_Group_Count[2], Work_Group_Size[0], Work_Group_Size[1], Work_Group_Size[2]);

				if (ImGui::InputInt ("Resolution Height", &m_OutputResolutionHeight)) {
					int tmp = MIN (MAX (m_OutputResolutionHeight, 100), 1000);
					if (m_OutputTexDimensions.y != tmp)
					{
						m_OutputTexDimensions.x *= (float (tmp)/m_OutputTexDimensions.y);
						m_OutputTexDimensions.y = tmp;
						glDeleteTextures (1, &m_ComputeShaderOutputTex);
						m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, m_OutputTexDimensions.x, m_OutputTexDimensions.y, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR);
						glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
					}
				}

				ImGui::DragFloat3 ("Camera Posn", &m_CameraPosn[0], 0.1f);
				ImGui::DragFloat2 ("Camera pitch(y), yaw(x)", &m_CameraPitchYaw[0]);
				ImGui::DragFloat ("Focus Distance", &m_FocusDist, 0.1f);

				ImGui::Checkbox ("Show Sphere Normal", &m_ShowNormals);

				ImGui::Text ("use W, A, S, D to move (while viewport focused)\nand UP, LEFT, DOWN, RIGHT to rotate camera\n");

				if (ImGui::InputInt ("Number Of Object Scene", (int *)((void *)&m_NumOfObjInGroup))) {
					if (m_GroupsIDBuffer.first.capacity () < m_NumOfObjInGroup) {
						m_GroupsIDBuffer.first.resize (m_NumOfObjInGroup);
					}
					if (m_GroupsDataBuffer.first.capacity () < m_NumOfObjInGroup) {
						m_GroupsDataBuffer.first.resize (m_NumOfObjInGroup);
					}
					while (m_GeometryGroup.size () < m_NumOfObjInGroup) {
						m_GeometryGroup.emplace_back (Geometry());
					}
				}
				if (ImGui::CollapsingHeader ("Objects In Scene")) {
					ImGui::Indent ();
					for (uint16_t i = 0; i < m_NumOfObjInGroup; i++) {
						ImGui::PushID (i);
						Geometry &obj = m_GeometryGroup[i];
						ImGui::Combo ("Geometry Type", (int*)&obj.Typ, "None\0Cuboid\0Ellipsoid\0");
						ImGui::InputFloat3 ("Position", &obj.Position[0]);
						if (ImGui::InputFloat3 ("Rotation", &obj.Rotation[0])) obj.ResetInvRotationMatrix ();
						ImGui::InputFloat3 ("Scale", &obj.Scale[0]);
						ImGui::ColorPicker3 ("Color", &obj.Color[0]);
						ImGui::Separator ();
						ImGui::PopID ();
					}
					ImGui::Unindent ();
				}

				ImGui::EndTabItem ();
			}
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Compute Shader Source"))) {

				OnImGuiComputeShaderSource ();

				ImGui::EndTabItem ();
			}
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Square Shader Source"))) {

				OnImGuiSqureShaderSource ();

				ImGui::EndTabItem ();
			}
			ImGui::EndTabBar ();
		}
		ImGui::End ();
	}
	void Groups::OnComputeShaderReload ()
	{
		m_FocusDistUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_FocusDist");
		m_CamDirnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraDirn");
		m_CamPosnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraPosn");
		m_ShowNormalsUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_ShowNormal");
		m_NumOfGeometryUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_NumOfObj");
	}
	void Groups::OnSquareShaderReload ()
	{}

	glm::vec3 Groups::FrontFromPitchYaw (float pitch, float yaw)
	{
		glm::vec3 front;
		front.x = cos (glm::radians (yaw)) * cos (glm::radians (pitch));
		front.y = sin (glm::radians (pitch));
		front.z = sin (glm::radians (yaw)) * cos (glm::radians (pitch));
		return glm::normalize (front);
	}
	void Groups::CopyObjBuffer ()
	{
		//bool resize_ID_tex = false, resize_data_tex = false;
		while (m_GroupsIDBuffer.first.size () < m_GeometryGroup.size ())
			m_GroupsIDBuffer.first.push_back (0.0f), m_GroupsIDBuffer.second = true;// , resize_ID_tex = true;
		while (m_GroupsDataBuffer.first.size () < m_GeometryGroup.size ())
			m_GroupsDataBuffer.first.emplace_back (GeometryBuff ()), m_GroupsDataBuffer.second = true;// , resize_data_tex = true;
		for (uint16_t i = 0; i < m_GeometryGroup.size (); i++) {
			m_GroupsDataBuffer.second |= m_GeometryGroup[i].FillBuffer (m_GroupsDataBuffer.first[i]);
		}
		for (uint16_t i = 0; i < m_GeometryGroup.size (); i++) {
			if (m_GroupsIDBuffer.first[i] != int(m_GeometryGroup[i].Typ))
				m_GroupsIDBuffer.first[i] = float (m_GeometryGroup[i].Typ), m_GroupsIDBuffer.second = true;
		}

		if (m_GroupsDataBuffer.second) {
			//if (resize_data_tex || m_GroupsDataBuffer.second) {
				glDeleteTextures (1, &m_GroupsDataBufferTex);
				m_GroupsDataBufferTex = 0;
			//}
			if (m_GroupsDataBufferTex == 0) {
				m_GroupsDataBufferTex = Helper::TEXTURE_2D::Upload (m_GroupsDataBuffer.first.data (), sizeof (GeometryBuff)/sizeof (float[3]), m_GroupsDataBuffer.first.size (), GL_RGB32F, GL_RGB, GL_FLOAT);
			}
			m_GroupsDataBuffer.second = false;
		}
		if (m_GroupsIDBuffer.second) {
			//if (resize_ID_tex || m_GroupsIDBuffer.second) { // currently i'm recreating texture as a change occurs in data it instead of changing data texture
				glDeleteTextures (1, &m_GroupsIDBufferTex);
				m_GroupsIDBufferTex = 0;
			//}
			if (m_GroupsIDBufferTex == 0) {
				m_GroupsIDBufferTex = Helper::TEXTURE_2D::Upload (m_GroupsIDBuffer.first.data (), m_GroupsIDBuffer.first.size (), 1, GL_R32F, GL_RED, GL_FLOAT);
			}
			m_GroupsIDBuffer.second = false;
		}
	}
};