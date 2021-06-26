#version 440 core
layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D img_output;

uniform ivec2 u_TileSize;
uniform ivec2 u_TileIndex;
uniform float u_FOV_y; // in radians
// uniform float u_FocusDist; // will be used for different purpose (depth of field).
uniform vec3 u_CameraPosn;
uniform vec3 u_CameraDirn;
uniform int u_NumOfBounce;
uniform int u_NumOfSamples;

uniform bool u_ShowNormal;


uniform int u_NumOfObj;
layout (r32f, binding = 0) uniform sampler2D u_ObjGroupIDTexture;
layout (rgba32f, binding = 1) uniform sampler2D u_ObjGroupDataTexture;

#define CUBOID     1 // const int 
#define ELLIPSOID  2 // const int 
#define PI 3.1415926538
const float PHI = PI*(3.0 - sqrt(5.0));

struct Ray
{
	vec3 Orig;
	vec3 Dirn;
};
struct Plane3D
{
	vec4 Eqn;
};
struct RayReturnData{
    vec2 scatteritivity; // refractivity, reflectivity, scatteritivity

    vec3 point;
    vec3 normal;
    vec3 reflected;

    vec3 material; // refractivity, reflectivity, refractive_index
    vec3 color;
};

float DistanceFrom(Ray ray, vec3 pt){
	return length(cross(pt - ray.Orig, ray.Dirn));
};
Ray CreateRay (vec3 pt1, vec3 pt2){
	return Ray (pt1, normalize (pt2 - pt1));
};
float t_RayXObj(Ray ray, int Obj_typ, vec3 scale){
	float t = -1.0;
	switch(Obj_typ){
		case ELLIPSOID:{
				vec3 _xyz2,_xyz3;
				_xyz2 = vec3(ray.Orig.x/scale.x, ray.Orig.y/scale.y, ray.Orig.z/scale.z);
				_xyz3 = vec3(ray.Dirn.x/scale.x, ray.Dirn.y/scale.y, ray.Dirn.z/scale.z);
				
				float half_b = dot(_xyz2,_xyz3);
				float a = dot(_xyz3,_xyz3);
				float c = dot(_xyz2,_xyz2) - 1;
				float determinant = half_b*half_b - a*c;
				if(determinant > 0){
					float _t[2];
					_t[0] = (-half_b - sqrt(determinant)) / a;
					_t[1] = (-half_b + sqrt(determinant)) / a;
					
					t = (_t[0] > _t[1] || _t[0] < 0) ? _t[1] : _t[0];// _t[1] < 0, it will be handled when returning from function
				}
			}; break;
		case CUBOID:{
			vec3 b_min = -scale*0.5f;
			vec3 b_max = +scale*0.5f;
			float t1 = (b_min[0] - ray.Orig[0])*(1.0/ray.Dirn[0]);
		    float t2 = (b_max[0] - ray.Orig[0])*(1.0/ray.Dirn[0]);
		
		    float tmin = min(t1, t2);
		    float tmax = max(t1, t2);
		
		    for (int i = 1; i < 3; ++i) {
		        t1 = (b_min[i] - ray.Orig[i])*(1.0/ray.Dirn[i]);
				t2 = (b_max[i] - ray.Orig[i])*(1.0/ray.Dirn[i]);
		        
				tmin = max(tmin, min(min(t1, t2), tmax));
		        tmax = min(tmax, max(max(t1, t2), tmin));
		    }
			t = tmax > tmin ? (tmin > 0 ? tmin : tmax) : -1.0;
		}; break;
	}
	return (t > 0 ? t : -1);
}
vec3 SurfaceNormal(float t, Ray ray, int Obj_typ, vec3 scale){
	vec3 HitPoint = ray.Orig + t*ray.Dirn;
	switch(Obj_typ){
		case ELLIPSOID:
			return vec3(HitPoint.x/scale.x*scale.x, HitPoint.y/scale.y*scale.y, HitPoint.z/scale.z*scale.z);
		case CUBOID:
			float min_dist = abs(HitPoint[0] - scale[0]*0.5);
			int index = 0;

			float dist = abs(HitPoint[0] + scale[0]*0.5);
			if(min_dist > dist)
				min_dist = dist, index = 1;
			
			for (int i = 1; i < 3; ++i) {
				dist = abs(HitPoint[i] - scale[i]*0.5);
				if(min_dist > dist)
					min_dist = dist, index = i*2;
				dist = abs(HitPoint[i] + scale[i]*0.5);
				if(min_dist > dist)
					min_dist = dist, index = i*2 + 1;
		    };
			vec3 result = vec3(0, 0, 0);
			result[index/2] = index%2 == 0 ? 1 : -1;
			return result;
	}
	return vec3(0,0,0);
}
vec3 Background_Color(vec3 ray_dirn){
	float t = (ray_dirn.y + 1.0)*0.5;
	return ((1.0 - t)*vec3 (1.0, 1.0, 1.0) + t*vec3 (0.3, 0.4, 1.0));
}

vec3 IntersectRayPlane(Ray ray, Plane3D plane){
	float t = -(plane.Eqn.x * (ray.Orig.x) + plane.Eqn.y * (ray.Orig.y) + plane.Eqn.z * (ray.Orig.z) + plane.Eqn.w) 
			/ (plane.Eqn.x * (ray.Dirn.x) + plane.Eqn.y * (ray.Dirn.y) + plane.Eqn.z * (ray.Dirn.z));

	float x = ray.Orig.x + ray.Dirn.x * t;
	float y = ray.Orig.y + ray.Dirn.y * t;
	float z = ray.Orig.z + ray.Dirn.z * t;

	return vec3( x, y, z );
}
vec3 ProjectPoint(Plane3D plane, vec3 point){
	return IntersectRayPlane(Ray(point, plane.Eqn.xyz), plane);
}
vec3 ProjectVector(Plane3D plane, vec3 vector){
	vec3 pt1 = ProjectPoint(plane, vec3(0));
	vec3 pt2 = ProjectPoint(plane, vector);
	return (pt2 - pt1);
}
Plane3D CreatePlane(vec3 norml, vec3 point){
	Plane3D plane;
	float w = -(norml.x * point.x + norml.y * point.y + norml.z * point.z) / length(norml);
	plane.Eqn = vec4(norml, w);
	return plane;
}

vec3 fibonacciHemiSpherePtDirn(int sample_index, int max_samples, float scatteritivity, vec3 focus_dirn){
	float y = 1 - (sample_index / float(max_samples - 1)); // Y goes 1 to 0 (since sample_index <= max_sample)
	float radius = sqrt(1 - y*y); // radius at Y

	float theta = PHI*sample_index;

	float x = cos(theta) * radius;
	float z = sin(theta) * radius;
	// x, y, z represents fibinachhi(pardon for spelling) sphere

	x *= scatteritivity; // scaling down sphere from 1.0 to scatteritivity
	y *= scatteritivity;
	z *= scatteritivity;

	vec3 y_cap = focus_dirn; // For rotating sphere up
	vec3 z_cap = normalize(cross(vec3(0, 1.0, 0), y_cap)); // any perpendicular 
	vec3 x_cap = normalize(cross(y_cap, z_cap));

	vec3 pt = focus_dirn + (x*x_cap + y*y_cap + z*z_cap); // rotated point & move origin of sphere to tip of reflection
	return normalize(pt);
}
float Schlick_Approx(float cosine, float ref_idx){
	// Use Schlick's approximation for reflectance.
    float r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine),5);
}

#define id_tex_size_x textureSize(u_ObjGroupIDTexture, 0).x // const int 
#define data_tex_size textureSize(u_ObjGroupDataTexture, 0) // const ivec2

// returns point of hit and associated normal, material, color at that point
RayReturnData LaunchRay(vec3 g_origin, vec3 g_dirn, float max_t_depth, float color_contribution){
    float min_t_depth = max_t_depth;
    mat3 rot_matrix_of_Intersected_obj, matrix;
    RayReturnData data;

	vec3 final_tranf_ray_dirn = vec3(0);
    for(int j = 0; j < u_NumOfObj; j++){
		int Type = int(texture(u_ObjGroupIDTexture, vec2((j+0.1)/id_tex_size_x, 0.1)).r);					   // texture fetch is expensive
		vec3 position  = texture(u_ObjGroupDataTexture, vec2(0.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		matrix[0] = texture(u_ObjGroupDataTexture, vec2(1.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		matrix[1] = texture(u_ObjGroupDataTexture, vec2(2.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		matrix[2] = texture(u_ObjGroupDataTexture, vec2(3.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		vec3 scale     = texture(u_ObjGroupDataTexture, vec2(4.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		
		vec3 transformed_ray_orig = matrix*(g_origin - position);
		vec3 transformed_ray_dirn = matrix*g_dirn;
		Ray transformed_ray = Ray(transformed_ray_orig, normalize(transformed_ray_dirn)); // insure dirn is normalized
		float t = t_RayXObj(transformed_ray, Type, scale);

        // Although these points are transformed, they can still be directly used to campare distance or depth
		if(min_t_depth > t && t > 0){
			data.normal = SurfaceNormal(t, transformed_ray, Type, scale);
			data.color    = texture(u_ObjGroupDataTexture, vec2(5.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
			data.material = texture(u_ObjGroupDataTexture, vec2(6.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
			data.scatteritivity = texture(u_ObjGroupDataTexture, vec2(7.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xy;// texture fetch is expensive

			final_tranf_ray_dirn = transformed_ray_dirn;

			rot_matrix_of_Intersected_obj = matrix;
			min_t_depth = t;
		}
	}
    if(min_t_depth < max_t_depth){
		// calculate reflection
		{
			bool ray_inside_Obj = dot(data.normal, final_tranf_ray_dirn) > 0;
			vec3 _normal = ray_inside_Obj ? -data.normal : data.normal; // normal towards the side of incident_ray
			
			data.reflected = reflect(final_tranf_ray_dirn, _normal); // In TIR no loss of light
			if(!ray_inside_Obj){
				// taking hit point as origin
				vec3 normal_to_i_n_r = normalize(cross(_normal, final_tranf_ray_dirn));
				vec3 normal_to_only_n = normalize(cross(normal_to_i_n_r, _normal));

				float scatteritivity = ray_inside_Obj ? data.scatteritivity.x : data.scatteritivity.y;
				
				float one_div_sqrt_one_plus_s_sqr = 1.0/sqrt(1.0 + scatteritivity*scatteritivity) ;
				vec3 max_reflect = scatteritivity*one_div_sqrt_one_plus_s_sqr*_normal + one_div_sqrt_one_plus_s_sqr *normal_to_only_n;// *normalize(cross(cross(_normal, final_tranf_ray_dirn), _normal));

				data.reflected = (dot(data.reflected, _normal) > dot(max_reflect, _normal)) ? data.reflected : max_reflect;
			}
		}
		rot_matrix_of_Intersected_obj = inverse(rot_matrix_of_Intersected_obj);

        data.point = g_origin + min_t_depth*g_dirn; // removing just a tinybit to ensure point is above the surface instead of being inside the shape
        data.normal = normalize(rot_matrix_of_Intersected_obj*data.normal);
        data.reflected = normalize(rot_matrix_of_Intersected_obj*data.reflected);
        data.color *= color_contribution;
    }else data.point = vec3(0), data.normal = vec3(0);
    return data;
};

#define stack_capacity 5

Ray   ray_stack[stack_capacity];
float ray_contribution_stack[stack_capacity];
float ray_medium_refractive_index_stack[stack_capacity];
int   ray_bounce_stack[stack_capacity];
int   ray_stack_size = 0;

void stack_push(Ray ray, float color_contribution, float incident_ray_medium_refractive_index, int bounced){
    if(ray_stack_size < stack_capacity){
        ray_stack[ray_stack_size] = ray;
        ray_contribution_stack[ray_stack_size] = color_contribution;
        ray_medium_refractive_index_stack[ray_stack_size] = incident_ray_medium_refractive_index;
        ray_bounce_stack[ray_stack_size] = bounced;
        ray_stack_size++;
    }
}
Ray stack_pop_out(out float color_contribution, out float refractive_index, out int bounced){
    if(ray_stack_size > 0){
        ray_stack_size--;
        color_contribution = ray_contribution_stack[ray_stack_size];
		refractive_index = ray_medium_refractive_index_stack[ray_stack_size];
        bounced = ray_bounce_stack[ray_stack_size];
        return ray_stack[ray_stack_size];
    }else return Ray(vec3(0), vec3(0));
}
// returns color captured by ray
vec3 LaunchRays(vec3 ray_origin, vec3 ray_dirn, int sample_index, const int max_samples, const int max_bounces){
    
    stack_push(Ray(ray_origin, ray_dirn), 1.0, 1.0, 0);
    
	vec3 sample_color = vec3(0);
	
	int skippast_ParentsForRI = 0;// Hack: since this is stack we can assume that parent ray is just above itself (if processing refracted ray, above it is reflected ray which is in same medium as parent ray)
	// so we have to keep track of skips for parent medium when undergoing refaction when ray is returning from medium to parent medium
    
	while(ray_stack_size > 0){

        float contribution, refractive_index;int bounced;
        Ray curr_ray = stack_pop_out(contribution, refractive_index, bounced);

        RayReturnData data = LaunchRay(curr_ray.Orig, curr_ray.Dirn, 32000, contribution);
        
        bool ray_hit = dot(data.normal, data.normal) > 0.9;// or less than 1 i.e 0, no point found
		
        // else add_color, and try push to stack
        sample_color += contribution*(ray_hit ? data.color : Background_Color(curr_ray.Dirn));

        if(bounced < max_bounces && ray_hit){
            bounced++; // curr_ray can bounce more

			{
				bool spawnReflected = false, spawnRefracted = false;
				vec3 refraction_dirn, reflection_dirn;
				
				float cos_theta = dot(data.normal, curr_ray.Dirn); // note theta can be both -tve(outer hit) and +tve(inner hit, check TIR)

				float sin_theta = sqrt(1.0 - cos_theta*cos_theta);
				float target_RI;{ // NOTE: parent material needed if refraction occurs, workaround is to use benifit of stack i.e. which is Last stacked Rays_RI (if again occurs 2nd last and so on.), make use of int skippast_ParentsForRI
					int ParentRI_Index = ray_stack_size - 1 - skippast_ParentsForRI;
					target_RI = cos_theta > 0 ? ((ParentRI_Index < 0) ? 1.0f : ray_medium_refractive_index_stack[ParentRI_Index]) : data.material.z;
				}
				float refraction_ratio_x_sin_theta = (refractive_index/target_RI)*sin_theta;

				float refracted_contribution = data.material.x, reflected_contribution = data.material.y;
				vec3 _normal = cos_theta > 0 ? data.normal : -data.normal; // away from incident
				if(cos_theta < 0){
					reflection_dirn = fibonacciHemiSpherePtDirn(sample_index, max_samples, data.scatteritivity.y, data.reflected), spawnReflected = true;

					float increase_in_reflected_contribution = refracted_contribution*Schlick_Approx(-cos_theta, (refractive_index/target_RI));
					refracted_contribution -= increase_in_reflected_contribution;
					reflected_contribution += increase_in_reflected_contribution;

				}else if (refraction_ratio_x_sin_theta > 1.0f)
					refraction_dirn = data.reflected, spawnReflected = true, reflected_contribution = 1.0;
				if(refraction_ratio_x_sin_theta <= 1.0f){ // when No TIR
				  	
					vec3 y_cap = _normal*cos_theta;
					vec3 x_cap = curr_ray.Dirn - y_cap;
				  	
					spawnRefracted = true;
					refraction_dirn = (refraction_ratio_x_sin_theta*_normal) + (sqrt(1.0 - refraction_ratio_x_sin_theta*refraction_ratio_x_sin_theta)*x_cap);
					refraction_dirn = fibonacciHemiSpherePtDirn(sample_index, max_samples, data.scatteritivity.x, refraction_dirn);
				}
				skippast_ParentsForRI = (spawnReflected && spawnRefracted) ? skippast_ParentsForRI - 1 : (spawnReflected ? skippast_ParentsForRI : (spawnRefracted ? skippast_ParentsForRI + 1 : 0)); 
				
				vec3 point;
				if(spawnReflected){
					point = data.point -0.000015*_normal; // rectify
					stack_push(Ray(point, reflection_dirn), contribution*reflected_contribution, refractive_index, bounced);
				}
				if(spawnRefracted){
					point = data.point +0.000015*_normal; // rectify
					stack_push(Ray(point, refraction_dirn), contribution*refracted_contribution, target_RI, bounced);
				}
			}

        }else skippast_ParentsForRI = 0; // refracted ray reached end
    }
    return sample_color;
}

#define world_up vec3(0, 1, 0)

vec4 out_Pixel (ivec2 pixel_coords, ivec2 img_size)
{
	const float aspectRatio = float (img_size.x)/img_size.y;
	
	int grid = 1;
	while(grid*grid < u_NumOfSamples)
		grid++;
	
	const float scr_x = (aspectRatio * (pixel_coords.x*2.0 - img_size.x)) / (2.0*img_size.x);
	const float scr_y = (pixel_coords.y*2.0 - img_size.y)/(2.0*img_size.y);
	const float del_scr_x = aspectRatio/float(img_size.x*grid);
	const float del_scr_y = 1.0/float(img_size.y*grid);
	
	vec3 final_color = vec3(0);
	int focus = 0, x = 0, y = 0;
    for (int samples_processed = 0; samples_processed < u_NumOfSamples; samples_processed++){
		vec3 ray_orig = u_CameraPosn;
		vec3 ray_dirn;
		ivec2 sample_indexs;{
			vec3 cam_right = cross (u_CameraDirn, world_up);
			vec3 cam_up = cross (cam_right, u_CameraDirn);
			if(focus < grid) {
				if(x == 0 && y == 0){
					focus++;
					x = focus, y = focus, sample_indexs = ivec2(focus,focus);;
				}else{
					if(x < y) y--, sample_indexs = ivec2(focus, y);
					else x--, sample_indexs = ivec2(x, focus);
				}
			}else return vec4 (final_color/samples_processed, 1.0);

			// replacement for u_FocusDist
			float screen_dist = 1.0 / (2.0*tan(u_FOV_y/2.0));

			ray_dirn = normalize(u_CameraDirn*screen_dist + cam_right*(scr_x + (del_scr_x*sample_indexs.x)) + cam_up*(scr_y + (del_scr_y*sample_indexs.y)));
		}
        
        // can add max_depth and depth fallof (amount of depth to reduce after each bounce)
		if(!u_ShowNormal){
        	final_color += LaunchRays(ray_orig, ray_dirn, samples_processed, u_NumOfSamples, u_NumOfBounce);
		}else{
			RayReturnData data = LaunchRay(ray_orig, ray_dirn, 32000, 1.0);
			final_color += data.normal;
		}
    }
    return vec4(final_color/u_NumOfSamples, 1.0);
}
void main ()
{
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2 (gl_WorkGroupID.xy);
	pixel_coords += ivec2(u_TileIndex.x*u_TileSize.x, u_TileIndex.y*u_TileSize.y);

	// base pixel color for image
	vec4 pixel = out_Pixel (pixel_coords, imageSize (img_output));

	// output to a specific pixel in the image
	imageStore (img_output, pixel_coords, pixel);
};