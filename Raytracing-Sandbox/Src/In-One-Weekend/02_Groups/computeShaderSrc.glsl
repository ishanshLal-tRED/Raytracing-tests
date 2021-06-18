#version 440 core
layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D img_output;

uniform float u_FocusDist;
uniform vec3 u_CameraPosn;
uniform vec3 u_CameraDirn;
uniform int u_NumOfSamples;
const int max_NumOfSamples = 16;

uniform int u_NumOfObj;
layout (r32f, binding = 0) uniform sampler2D u_ObjGroupIDTexture;
layout (rgba32f, binding = 1) uniform sampler2D u_ObjGroupDataTexture;


const int CUBOID = 1;
const int ELLIPSOID = 2;
//const int NumOfObj = textureSize(u_ObjGroupIDTexture, 0).x;
struct Ray
{
	vec3 Orig;
	vec3 Dirn;
};

uniform bool u_ShowNormal;
uniform bool u_Cull_Front;
uniform bool u_Cull_Back;

float DistanceFrom(Ray ray, vec3 pt){
	return length(cross(pt - ray.Orig, ray.Dirn));
};
Ray CreateRay (vec3 pt1, vec3 pt2)
{
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
					if(!u_Cull_Back && !u_Cull_Front){ // find min
						t = _t[0];
						if(_t[0] > _t[1] || _t[0] < 0) // _t[1] < 0, it will be handled when returning from function
							t = _t[1];
						break;
					}else if(!u_Cull_Front){
						t = min(_t[0], _t[1]);
					}else if(!u_Cull_Back){
						t = max(_t[0], _t[1]); 
					}
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
			if(tmax > max(tmin, 0.0)){
				if(!u_Cull_Back && !u_Cull_Front){				
					t = tmin > 0 ? tmin : tmax;
				}else if(!u_Cull_Front){
					t = tmin;
				}else if(!u_Cull_Back){
					t = tmax; 
				}
			}
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
			// result = vec3(t/20, t/15, t/10); // for some beautiful result
			return result;
	}
	return vec3(0,0,0);
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
	const int num_of_samples = max(min(u_NumOfSamples, max_NumOfSamples), 1); // atleast 1 sample

	//vec3 point_in_scr_space = u_CameraPosn + u_CameraDirn*u_FocusDist + cam_right*scr_x + cam_up*scr_y;
	// instead of finding point in scr space early, they will be calculated on per sample basis;
// Color
	vec3 final_color;{ // background
		float ray_Dirn_y = normalize(u_CameraDirn*u_FocusDist + cam_right*scr_x + cam_up*scr_y).y; // how expenive
		float t = (ray_Dirn_y + 1.0)*0.5;
		final_color = ((1.0 - t)*vec3 (1.0, 1.0, 1.0) + t*vec3 (0.3, 0.4, 1.0));
	}
	vec3 sample_color[max_NumOfSamples]; // hard coding max samples
	for(int i = 0; i < num_of_samples; i++)
		sample_color[i] = final_color;
	
//Depth Testing
	vec3 position, scale, obj_color;
	mat3 matrix;
	int Type;
	float min_t_depth[max_NumOfSamples]; // FLT_MAX
	for(int i = 0; i < num_of_samples; i++)
		min_t_depth[i] = 32000;

	int grid = 1;
	while(grid*grid < num_of_samples)
		grid++;
	float del_scr_x = aspectRatio/float(img_size.x*grid);
	float del_scr_y = 1.0/float(img_size.y*grid);

	const int id_tex_size_x = textureSize(u_ObjGroupIDTexture, 0).x;
	const ivec2 data_tex_size = textureSize(u_ObjGroupDataTexture, 0);
	for(int i = 0; i < u_NumOfObj; i++){
		Type = int(texture(u_ObjGroupIDTexture, vec2((i+0.1)/id_tex_size_x, 0.1)).r);					   // texture fetch is expensive
		position  = texture(u_ObjGroupDataTexture, vec2(0.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		matrix[0] = texture(u_ObjGroupDataTexture, vec2(1.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		matrix[1] = texture(u_ObjGroupDataTexture, vec2(2.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		matrix[2] = texture(u_ObjGroupDataTexture, vec2(3.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		scale     = texture(u_ObjGroupDataTexture, vec2(4.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
		obj_color = texture(u_ObjGroupDataTexture, vec2(5.1/data_tex_size.x, (i+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive

		// Although these points are transformed, they can still be directly used to 
		vec3 transformed_orig = matrix*(u_CameraPosn - position);
		vec3 transformed_dirn = matrix*u_CameraDirn;
		vec3 transformed_right = matrix*cam_right;
		vec3 transformed_up = matrix*cam_up;

		int focus = 0, x = 0, y = 0;
		int samples_processed = 0;
		ivec2 process_indexs = ivec2(0, 0);
		while(focus < grid) {
			
			vec3 transformed_point_in_scr = transformed_orig + transformed_dirn*u_FocusDist + transformed_right*(scr_x + (del_scr_x*process_indexs.x)) + transformed_up*(scr_y + (del_scr_y*process_indexs.y));
			Ray sample_ray = CreateRay (transformed_orig, transformed_point_in_scr);
			
			float t = t_RayXObj(sample_ray, Type, scale);
			if(min_t_depth[samples_processed] > t && t > 0){
				if(u_ShowNormal)
					sample_color[samples_processed] = SurfaceNormal(t, sample_ray, Type, scale);
				else
					sample_color[samples_processed] = obj_color;
				min_t_depth[samples_processed] = t;
			}

			if(x == 0 && y == 0){
				focus++;
				x = focus, y = focus, process_indexs = ivec2(focus,focus);;
			}else{
				if(x < y) y--, process_indexs = ivec2(focus, y);
				else x--, process_indexs = ivec2(x, focus);
			}
			samples_processed++;
			if(samples_processed >= num_of_samples)
				break;
		}
	}
	final_color = vec3(0);
	for(int i = 0; i < num_of_samples; i++){
		final_color += sample_color[i];
	}
	final_color /= float(num_of_samples);
	return vec4 (final_color, 1.0);
}
void main ()
{
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2 (gl_GlobalInvocationID.xy);

	// base pixel color for image
	vec4 pixel = out_Pixel (pixel_coords);

	// output to a specific pixel in the image
	imageStore (img_output, pixel_coords, pixel);
};