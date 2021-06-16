#include "Sphere.h"
#include <GLCoreUtils.h>
#include <GLCore/Core/KeyCodes.h>
namespace In_One_Weekend
{
	const char *Sphere::s_ComputeShader = R"(
#version 440 core
layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D img_output;

uniform float u_FocusDist;
uniform vec3 u_CameraPosn;
uniform vec3 u_CameraDirn;

struct Ray
{
	vec3 Orig;
	vec3 Dirn;
};
struct Plane3D
{
	vec4 Eqn;
};
struct Sphere 
{
	float Radius;
	vec3 Centre;
};

uniform bool u_ShowNormal;
uniform Sphere u_Sphere;

float DistanceFrom(Ray ray, vec3 pt){
	return length(cross(pt - ray.Orig, ray.Dirn));
};
float DistanceFrom(Plane3D plane, vec3 pt){
	float num = (plane.Eqn.x * pt.x + plane.Eqn.y * pt.y + plane.Eqn.z * pt.z) + plane.Eqn.w;
	float den = length(plane.Eqn.xyz);
	return num / den;
}
Ray CreateRay (vec3 pt1, vec3 pt2)
{
	return Ray (pt1, normalize (pt2 - pt1));
};
Plane3D CreatePlane (vec3 pt1, vec3 pt2, vec3 pt3)
{
	vec4 param = vec4(normalize(cross(pt2 - pt1, pt3 - pt1)), 0);
	param.w = -(param.x * pt1.x + param.y * pt1.y + param.z * pt1.z);
	
	return Plane3D (param);
};
Plane3D CreatePlane (Ray ray, vec3 pt)
{
	vec4 param = vec4(normalize(cross(ray.Dirn, ray.Orig - pt)), 0);
	param.w = -(param.x * pt.x + param.y * pt.y + param.z * pt.z);
	
	return Plane3D (param);
}
vec3 Intersect(Ray ray, Plane3D plane)
{
	float t = -(plane.Eqn.x * (ray.Orig.x) + plane.Eqn.y * (ray.Orig.y) + plane.Eqn.z * (ray.Orig.z) + plane.Eqn.w) 
			/ (plane.Eqn.x * (ray.Dirn.x) + plane.Eqn.y * (ray.Dirn.y) + plane.Eqn.z * (ray.Dirn.z));

	float x = ray.Orig.x + ray.Dirn.x * t;
	float y = ray.Orig.y + ray.Dirn.y * t;
	float z = ray.Orig.z + ray.Dirn.z * t;

	return vec3( x, y, z );
}

float t_RayXPlane (Ray ray, Plane3D plane)
{
	float t = -(plane.Eqn.x * (ray.Orig.x) + plane.Eqn.y * (ray.Orig.y) + plane.Eqn.z * (ray.Orig.z) + plane.Eqn.w) 
			/ (plane.Eqn.x * (ray.Dirn.x) + plane.Eqn.y * (ray.Dirn.y) + plane.Eqn.z * (ray.Dirn.z));
	return t;
}
float RayXPlane (Ray ray, Plane3D plane)
{
	float t = t_RayXPlane (ray, plane);
	return (t > 0 ? length(ray.Dirn*t) : -1.0);
}
float t_RayXSphere (Ray ray, Sphere sphere)
{
	vec3 ray_to_sphere = ray.Orig - sphere.Centre;
	float half_b = dot(ray.Dirn, ray_to_sphere);
	float a = dot(ray.Dirn, ray.Dirn);
	float c = dot(ray_to_sphere, ray_to_sphere) - sphere.Radius*sphere.Radius;
	
	float determinant = half_b*half_b - a*c;

	return (determinant > 0 && half_b < 0 ? ((-half_b - sqrt(determinant))/a) : -1.0);
}
float DepthRayXSphere (Ray ray, Sphere sphere)
{
	float t = t_RayXSphere (ray, sphere);
	return (t > 0 ? length(ray.Dirn*t) : -1.0);
}
vec3 RayColor (Ray ray)
{
	float t = (ray.Dirn.y + 1.0)*0.5;
	return ((1.0 - t)*vec3 (1.0, 1.0, 1.0) + t*vec3 (0.3, 0.4, 1.0));
}

vec4 out_Pixel (ivec2 pixel_coords)
{
	ivec2 img_size = imageSize (img_output);
	float aspectRatio = float (img_size.x)/img_size.y;
	vec3 world_up = vec3 (0, 1, 0);
	vec3 cam_right = cross (u_CameraDirn, world_up);
	vec3 cam_up = cross (cam_right, u_CameraDirn);
	float scr_x = (pixel_coords.x*2.0 - img_size.x)/(2.0*img_size.x);
	scr_x *= aspectRatio;
	float scr_y = (pixel_coords.y*2.0 - img_size.y)/(2.0*img_size.y);

	vec3 point_in_scr_space = u_CameraPosn + u_CameraDirn*u_FocusDist + cam_right*scr_x + cam_up*scr_y;
	
// Ray and other stuff
	Plane3D infinite_floor = CreatePlane (vec3 (0, -2, 0), vec3 (0, -2, 1), vec3 (1, -2, 0));
	Ray cam_ray = CreateRay (u_CameraPosn, point_in_scr_space);
	
// Color
	vec3 color = RayColor (cam_ray); // background
	float min_depth = 10000; // FLT_MAX
	
//Depth Testing with t instead of proper depth
	{
		float t_depth = t_RayXPlane (cam_ray, infinite_floor); // with plane
		if(min_depth > t_depth && t_depth > 0){
			color = vec3 (0.8, 0.1, 0.7f); // plane color
			min_depth = t_depth;
		}
	}{
		float t_depth = t_RayXSphere (cam_ray, u_Sphere); // with sphere
		if(min_depth > t_depth && t_depth > 0){
			vec3 intersection_pt = cam_ray.Orig + cam_ray.Dirn*t_depth;
			color = u_ShowNormal ? normalize(intersection_pt - u_Sphere.Centre) : vec3 (1, 0, 0);
			min_depth = t_depth;
		}
	}
	return vec4 (color, 1.0);
}
void main ()
{
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2 (gl_GlobalInvocationID.xy);

	// base pixel color for image
	vec4 pixel = out_Pixel (pixel_coords);

	// output to a specific pixel in the image
	imageStore (img_output, pixel_coords, pixel);
}
)";
	void Sphere::OnAttach ()
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

			m_ComputeShaderOutputTex = m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, tex_w, tex_h, GL_RGBA32F, GL_RGBA, GL_FLOAT);

			// (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
			glBindImageTexture (0, m_ComputeShaderOutputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		}
	}
	void Sphere::OnDetach ()
	{
		DeleteQuadVAO ();

		glDeleteTextures (1, &m_ComputeShaderOutputTex);
		m_ComputeShaderOutputTex = 0;

		DeleteComputeShader ();
		DeleteSquareShader ();
	}
	void Sphere::OnUpdate (GLCore::Timestep ts)
	{
		glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		{ // launch compute shaders!
			glUseProgram (m_ComputeShaderProgID);

			glUniform1i (m_ShowNormalsUniLoc, int(m_ShowNormals));
			glUniform3f (m_SpherePosnUniLoc, m_Sphere.x, m_Sphere.y, m_Sphere.z);
			glUniform1f (m_SphereRadiusUniLoc, m_Sphere.w);

			glUniform1f (m_FocusDistUniLoc, m_FocusDist);
			glm::vec3 cam_dirn = FrontFromPitchYaw (m_CameraPitchYaw.x, m_CameraPitchYaw.y);
			glUniform3f (m_CamDirnUniLoc, cam_dirn.x, cam_dirn.y, cam_dirn.z);
			glUniform3f (m_CamPosnUniLoc, m_CameraPosn.x, m_CameraPosn.y, m_CameraPosn.z);
			
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
	void Sphere::OnEvent (GLCore::Event &event)
	{
		GLCore::EventDispatcher dispatcher (event);
		dispatcher.Dispatch<GLCore::LayerViewportResizeEvent> (
			[&](GLCore::LayerViewportResizeEvent &e) {
				glDeleteTextures (1, &m_ComputeShaderOutputTex);

				m_OutputTexDimensions.x = (float(e.GetWidth ())/e.GetHeight ())*100, m_OutputTexDimensions.y = 100;
				
				m_ComputeShaderOutputTex = Helper::TEXTURE_2D::Upload (nullptr, m_OutputTexDimensions.x, m_OutputTexDimensions.y, GL_RGBA32F, GL_RGBA, GL_FLOAT);
				
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
						pitch = MIN (pitch + 0.3f,  89.0f); break;
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
	void Sphere::OnImGuiRender ()
	{
		using namespace GLCore;
		ImGui::Begin (ImGuiLayer::UniqueName ("Just a window"));
		if (ImGui::BeginTabBar (ImGuiLayer::UniqueName ("Shaders Content"))) {
			if (ImGui::BeginTabItem (ImGuiLayer::UniqueName ("Settings"))) {

				ImGui::Text ("Max Compute Work Group\n Count: %d, %d, %d\n Size:  %d, %d, %d", Work_Group_Count[0], Work_Group_Count[1], Work_Group_Count[2], Work_Group_Size[0], Work_Group_Size[1], Work_Group_Size[2]);
				
				ImGui::DragFloat3 ("Camera Posn", &m_CameraPosn[0], 0.1f);
				ImGui::DragFloat2 ("Camera pitch(y), yaw(x)", &m_CameraPitchYaw[0]);
				ImGui::DragFloat ("Focus Distance", &m_FocusDist, 0.1f);
				
				ImGui::Checkbox ("Show Sphere Normal", &m_ShowNormals);
				
				ImGui::DragFloat3 ("Sphere Position", &m_Sphere[0]);
				ImGui::DragFloat ("Sphere Radius", &m_Sphere[3]);

				ImGui::Text ("use W, A, S, D to move (while viewport focused)\nand UP, LEFT, DOWN, RIGHT to rotate camera\n");
				
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
	void Sphere::OnComputeShaderReload ()
	{
		m_FocusDistUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_FocusDist");
		m_CamDirnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraDirn");
		m_CamPosnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_CameraPosn");
		m_ShowNormalsUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_ShowNormal");
		m_SphereRadiusUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Sphere.Radius");
		m_SpherePosnUniLoc = glGetUniformLocation (m_ComputeShaderProgID, "u_Sphere.Centre");
	}
	void Sphere::OnSquareShaderReload ()
	{}

	glm::vec3 Sphere::FrontFromPitchYaw (float pitch, float yaw)
	{
		glm::vec3 front;
		front.x = cos (glm::radians (yaw)) * cos (glm::radians (pitch));
		front.y = sin (glm::radians (pitch));
		front.z = sin (glm::radians (yaw)) * cos (glm::radians (pitch));
		return glm::normalize (front);
	}
};