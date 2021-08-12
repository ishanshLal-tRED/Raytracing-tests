#version 440 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (rgba8, binding = 0) uniform image2D img_output;
layout (r32f, binding = 1) uniform image2D dpth_out;

uniform ivec2 u_ImgOffset;

uniform struct
{
	vec3  Position;
	vec3  Direction;
	float FOV_y;
	int   NumOfFocusPts;
	float FocusDist[3];
	float Aperture;
} u_Camera;

uniform int u_NumOfBounce;

layout (rgba32f, binding = 0) uniform sampler2D u_SceneHierarchy;

layout (rgba32f, binding = 1) uniform sampler2D u_GeometryData;
// [vec3 posn, mat3 rotn_matrix, vec3 scale, int type, vec3 color, float refractive_index, float refractivity, float reflectivity, vec2 scatteritivity]

#define CUBOID     1 // const int 
#define ELLIPSOID  2 // const int 
#define PI 3.1415926538
const float PHI = PI*(3.0 - sqrt(5.0));

////////// HELPER //////
vec3 Background_Color(vec3 ray_dirn) {
	float t = (ray_dirn.y + 1.0)*0.5;
	return ((1.0 - t)*vec3 (1.0, 1.0, 1.0) + t*vec3 (0.3, 0.4, 1.0));
}
vec2 sunflower_distr(uint pt, uint max_pt){
	if(pt == 0){
		return vec2(0);
	}
	float b = round(2*sqrt(max_pt));

	float r = (pt > max_pt - b) ? 1.0 : sqrt((pt - 0.5)/(max_pt - (b+1)/2.0));
	float theta = PHI*pt;
	return vec2(r*cos(theta), r*sin(theta));
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
////////////////////////


struct Ray
{
	vec3 Orig;
	vec3 Dirn;
};
struct Material{
    vec3 color;
	
    float refractive_index, refractivity, reflectivity;
	// refractivity: how much of light gonna refract away
	// reflectivity: how much of light gonna reflect away
    
	vec2 scatteritivity; // scatteritivity (refracted, reflected): amount by which light gonna scatter/deviate
};
struct RayReturnData{
    vec3 point;
    vec3 normal;
    vec3 reflected;

	Material material;
};

float DistanceFrom(Ray ray, vec3 pt){
	return length(cross(pt - ray.Orig, ray.Dirn));
};
Ray CreateRay (vec3 pt1, vec3 pt2){
	return Ray (pt1, normalize (pt2 - pt1));
};
bool TestIntersectAABB(vec3 minimum, vec3 maximum, Ray ray, float limit) { // return true only if ray intersected and less than limit otherwise ignore
	float t1 = (minimum[0] - ray.Orig[0])*(1.0/ray.Dirn[0]);
	float t2 = (maximum[0] - ray.Orig[0])*(1.0/ray.Dirn[0]);
	
	float t_min = min(t1, t2);
	float t_max = max(t1, t2);

	for (int i = 0; i < 3; i++) {
        float _t1 = (minimum[i] - ray.Orig[i]) / ray.Dirn[i];
		float _t2 = (maximum[i] - ray.Orig[i]) / ray.Dirn[i];

		t1 = min(_t1, _t2);
		t2 = max(_t1, _t2);

		t_min = max(t1, t_min);
        t_max = min(t2, t_max);

        if (t_max <= t_min)
            return false;
    }
    return limit > 0.0f ? limit > t_min : true;
}

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

float Schlick_Approx(float cosine, float ref_idx){
	// Use Schlick's approximation for reflectance.
    float r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine),5);
}

const float node_data_size = textureSize(u_SceneHierarchy, 0).x;
// vec3 BB_min, vec3 BB_max, vec3 [leaf ? LeftChildIndex : 0, leaf ? RightChildIndex : 0, leaf ? correspondingObjID : ParentIndex] i.e 3 RGB32f
const float node_tree_size = textureSize(u_SceneHierarchy, 0).y; // or tree length

#define stack_capacity 40
struct FLT_STACK{
	float data[stack_capacity];
	uint size;
}; // use define for creating functions

#define STK_PUSH(stack, val)              if(stack.size<stack_capacity){stack.data[stack.size]=float(val);stack.size++;}
#define STK_POP_TO(stack, store)          if(stack.size>0){stack.size--;store=stack.data[stack.size];}
// Ray is float[6], contrib is float, bounce is int
#define STK_PUSH_RAY_DATA(stack, ray, contrib, bounced)     if(stack.size<(stack_capacity-7)){\
																stack.data[stack.size]=ray.Orig.x;stack.size++;\
																stack.data[stack.size]=ray.Orig.y;stack.size++;\
																stack.data[stack.size]=ray.Orig.z;stack.size++;\
																stack.data[stack.size]=ray.Dirn.x;stack.size++;\
																stack.data[stack.size]=ray.Dirn.y;stack.size++;\
																stack.data[stack.size]=ray.Dirn.z;stack.size++;\
																stack.data[stack.size]=contrib;stack.size++;\
																stack.data[stack.size]=float(bounced);stack.size++;\
															}
#define STK_POP_RAY_DATA_TO(stack, ray, contrib, bounced)   if((stack.size-7)>0){\
																stack.size--;bounced=int(stack.data[stack.size]);\
																stack.size--;contrib=stack.data[stack.size];\
																stack.size--;ray.Dirn.z=stack.data[stack.size];\
																stack.size--;ray.Dirn.y=stack.data[stack.size];\
																stack.size--;ray.Dirn.x=stack.data[stack.size];\
																stack.size--;ray.Orig.z=stack.data[stack.size];\
																stack.size--;ray.Orig.y=stack.data[stack.size];\
																stack.size--;ray.Orig.x=stack.data[stack.size];\
															}else ray.Orig=vec3(0),ray.Dirn=vec3(0),contrib=0,bounced=0;
															
#define MAX_T_DEPTH 100000
vec3 LaunchRay(vec3 origin, vec3 direction)
{
	FLT_STACK flt_stack;// will store everything that needs to be stacked
	STK_PUSH_RAY_DATA(flt_stack, Ray(origin, direction), 1.0, 0);
	vec3 final_color = vec3(0);
	// Note: For refractive_index of material, we'll extracting information from scene hierarchy, thats the most logical thing to do instead of some fancy stuff 

	while(flt_stack.size > 0){
		Ray currRay; float color_contribution; int bounced;
		STK_POP_RAY_DATA_TO(flt_stack, currRay, color_contribution, bounced);

		RayReturnData ray_data; bool ray_hit = false;
		{// Launch current ray // No ray pushing, fill ray_data
			bool reverse_search = dot(currRay.Dirn, vec3(1,1,1)) < 0;
			uint stack_tail = flt_stack.size; // last element will be here
			float min_t_depth = MAX_T_DEPTH;
			mat3 final_rotn_matrix;
			vec3 final_tranf_ray_dirn = vec3(0); // ray direction after being transformed
			float IntersectionCandidate = 0; // index
			float finalCandidate = 0; // index
			{// Find Next Candidate
				vec3 BB_min = texture(u_SceneHierarchy, vec2(0.1f/node_data_size, (0.1f)/node_tree_size)).xyz;
				vec3 BB_max = texture(u_SceneHierarchy, vec2(1.1f/node_data_size, (0.1f)/node_tree_size)).xyz;
				if(TestIntersectAABB(BB_min, BB_max, currRay, min_t_depth))
					STK_PUSH(flt_stack, 0);
				while(flt_stack.size > stack_tail){
					float node;
					STK_POP_TO(flt_stack, node);
					vec3 data = texture(u_SceneHierarchy, vec2(2.1f/node_data_size, (node + 0.1f)/node_tree_size)).xyz;
					if(data.x < 1 && data.y < 1){ // leaf, found
						IntersectionCandidate = data.z; 
						// no need to push it back
						break;
					}
					// else

					float back = data.y, front = data.x;
					if(reverse_search)
						back = data.x, front = data.y;
					BB_min = texture(u_SceneHierarchy, vec2(0.1f/node_data_size, (back + 0.1f)/node_tree_size)).xyz;
					BB_max = texture(u_SceneHierarchy, vec2(1.1f/node_data_size, (back + 0.1f)/node_tree_size)).xyz;
					if(TestIntersectAABB(BB_min, BB_max, currRay, min_t_depth))
						STK_PUSH(flt_stack, back);
					BB_min = texture(u_SceneHierarchy, vec2(0.1f/node_data_size, (front + 0.1f)/node_tree_size)).xyz;
					BB_max = texture(u_SceneHierarchy, vec2(1.1f/node_data_size, (front + 0.1f)/node_tree_size)).xyz;
					if(TestIntersectAABB(BB_min, BB_max, currRay, min_t_depth))
						STK_PUSH(flt_stack, front);
				}
			}
			// Start
			while(IntersectionCandidate > -0.5){
				vec3 position, scale; mat3 rotn_matrix; 
				
				// TODO this should be after last_position though
				int type;
				
				// TODO motion blur
				//vec3 last_position;
				{
					vec4 data;
					data = texture(u_GeometryData, vec2(0.1 / textureSize(u_GeometryData, 0).x, (IntersectionCandidate + 0.1) / textureSize(u_GeometryData, 0).y));
					position = data.xyz;
					rotn_matrix[0].x = data.w;

					data = texture(u_GeometryData, vec2(1.1 / textureSize(u_GeometryData, 0).x, (IntersectionCandidate + 0.1) / textureSize(u_GeometryData, 0).y));
					rotn_matrix[0].y = data.x, rotn_matrix[0].z = data.y;
					rotn_matrix[1].y = data.z, rotn_matrix[1].z = data.w;

					data = texture(u_GeometryData, vec2(2.1 / textureSize(u_GeometryData, 0).x, (IntersectionCandidate + 0.1) / textureSize(u_GeometryData, 0).y));
					rotn_matrix[1].z = data.x;
					rotn_matrix[2] = data.yzw;

					data = texture(u_GeometryData, vec2(3.1 / textureSize(u_GeometryData, 0).x, (IntersectionCandidate + 0.1) / textureSize(u_GeometryData, 0).y));
					scale = data.xyz;
					
					type = int(data.w);
				}
				// transform
				float t;
				rotn_matrix = transpose(rotn_matrix); // inverting rotation matrix
				vec3 transf_dirn = rotn_matrix*currRay.Dirn;
				// user specific
				Ray transf_ray = Ray(rotn_matrix*(currRay.Orig - position), transf_dirn);
				t = t_RayXObj(transf_ray, type, scale);
				if(t < min_t_depth && t > 0){
					ray_data.normal = SurfaceNormal(t, transf_ray, type, scale);
					finalCandidate = IntersectionCandidate;
					final_rotn_matrix = transpose(rotn_matrix); // invert_back
					final_tranf_ray_dirn = transf_dirn;
				}
				IntersectionCandidate = -1; // Invalidate
				while(flt_stack.size > stack_tail) { // next Intersection Candidate
					float node;
					STK_POP_TO(flt_stack, node);
					vec3 data = texture(u_SceneHierarchy, vec2(2.1f/node_data_size, (node + 0.1f)/node_tree_size)).xyz;
					if(data.x < 1 && data.y < 1){ // leaf, found
						IntersectionCandidate = data.z;
						break;
					}
					// else

					float back = data.y, front = data.x;
					if(reverse_search)
						back = data.x, front = data.y;
					vec3 BB_min, BB_max;
					BB_min = texture(u_SceneHierarchy, vec2(0.1f/node_data_size, (back + 0.1f)/node_tree_size)).xyz;
					BB_max = texture(u_SceneHierarchy, vec2(1.1f/node_data_size, (back + 0.1f)/node_tree_size)).xyz;
					if(TestIntersectAABB(BB_min, BB_max, currRay, min_t_depth))
						STK_PUSH(flt_stack, back);
					BB_min = texture(u_SceneHierarchy, vec2(0.1f/node_data_size, (front + 0.1f)/node_tree_size)).xyz;
					BB_max = texture(u_SceneHierarchy, vec2(1.1f/node_data_size, (front + 0.1f)/node_tree_size)).xyz;
					if(TestIntersectAABB(BB_min, BB_max, currRay, min_t_depth))
						STK_PUSH(flt_stack, front);
				}
			}
			if(min_t_depth < MAX_T_DEPTH){ // Intersected
				{
					vec4 data;
					data = texture(u_GeometryData, vec2(4.1 / textureSize(u_GeometryData, 0).x, (IntersectionCandidate + 0.1) / textureSize(u_GeometryData, 0).y));
					ray_data.material.color = data.xyz;
					ray_data.material.refractive_index = data.w;
					data = texture(u_GeometryData, vec2(5.1 / textureSize(u_GeometryData, 0).x, (IntersectionCandidate + 0.1) / textureSize(u_GeometryData, 0).y));
					ray_data.material.refractivity = data.x;
					ray_data.material.reflectivity = data.y;
					ray_data.material.scatteritivity = data.zw;
				}
				{
					bool ray_inside_Obj = dot(ray_data.normal, final_tranf_ray_dirn) > 0;
					vec3 _normal = ray_inside_Obj ? -ray_data.normal : ray_data.normal; // normal towards the side of incident_ray
					
					ray_data.reflected = reflect(final_tranf_ray_dirn, _normal); // In TIR no loss of light
					if(!ray_inside_Obj){
						// taking hit point as origin
						vec3 normal_to_i_n_r = normalize(cross(_normal, final_tranf_ray_dirn));
						vec3 normal_to_only_n = normalize(cross(normal_to_i_n_r, _normal));

						float scatteritivity = ray_inside_Obj ? ray_data.material.scatteritivity.x : ray_data.material.scatteritivity.y;
						
						float one_div_sqrt_one_plus_s_sqr = 1.0/sqrt(1.0 + scatteritivity*scatteritivity) ;
						vec3 max_reflect = scatteritivity*one_div_sqrt_one_plus_s_sqr*_normal + one_div_sqrt_one_plus_s_sqr *normal_to_only_n;// *normalize(cross(cross(_normal, final_tranf_ray_dirn), _normal));

						ray_data.reflected = (dot(ray_data.reflected, _normal) > dot(max_reflect, _normal)) ? ray_data.reflected : max_reflect;
					}
				}
				ray_data.point = currRay.Orig + currRay.Dirn*min_t_depth;
				ray_data.normal = final_rotn_matrix*ray_data.normal;
				ray_data.reflected = final_rotn_matrix*ray_data.reflected;
				ray_hit = true;
			}else{
				ray_data.material.color = Background_Color(currRay.Dirn);
			}
			// End
			while(flt_stack.size > stack_tail){
				float temp;
				STK_POP_TO(flt_stack, temp);
			}
		}

		final_color += color_contribution*ray_data.material.color;
		if(bounced < u_NumOfBounce && ray_hit){
			
		}
	}
	return final_color;
}

vec4 out_pixel(ivec2 pixelCoord, ivec2 imageSize) {
	vec3 ray_orig = vec3(0); // origin
	vec3 ray_dirn = vec3(0); // direction
	{
		float aspectRatio = float(imageSize.x)/imageSize.y;
		uint numOfSamples = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
		
		vec2 rel_posn = vec2(0);
		if(u_Camera.NumOfFocusPts > 0)
			rel_posn = sunflower_distr(gl_LocalInvocationIndex, numOfSamples)*u_Camera.Aperture*0.5;
		const vec3 world_up = vec3(0, 1, 0);
		vec3 cam_right = cross (u_Camera.Direction, world_up          );
		vec3 cam_up    = cross (cam_right         , u_Camera.Direction);

		ray_orig         = u_Camera.Position + (rel_posn.x * cam_right) + (rel_posn.y * cam_up); // Collector plate/lens
		vec3 lookat_posn = u_Camera.Position + u_Camera.Direction * u_Camera.FocusDist[0]; // no focus

		vec3 lookat_dirn = normalize(lookat_posn - ray_orig);

		vec3 _right = cross (lookat_dirn, world_up);
		vec3 _up    = cross (_right, lookat_dirn);
		vec3 _x = ((float(pixelCoord.x)/imageSize.x) - 0.5) * _right * aspectRatio;
		vec3 _y = ((float(pixelCoord.y)/imageSize.y) - 0.5) * _up;
		ray_dirn = normalize(lookat_dirn + _x + _y);
	}
	// Sample data(sample index and total samples) will be calculated locally
	return vec4(LaunchRay(ray_orig, ray_dirn), 1.0);
}

void main ()
{
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2 (gl_WorkGroupID.xy);
	pixel_coords += u_ImgOffset;

	// base pixel color for image
	vec4 pixel = out_pixel(pixel_coords, imageSize (img_output));

	// output to a specific pixel in the image
	imageStore (img_output, pixel_coords, pixel);
};