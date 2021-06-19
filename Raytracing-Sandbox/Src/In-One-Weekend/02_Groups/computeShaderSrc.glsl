#version 440 core
layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D img_output;

uniform float u_FocusDist;
uniform vec3 u_CameraPosn;
uniform vec3 u_CameraDirn;
uniform int u_NumOfBounce;
uniform int u_NumOfSamples;

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
	const vec3 world_up = vec3 (0, 1, 0);
	float scr_x = (pixel_coords.x*2.0 - img_size.x)/(2.0*img_size.x);
	scr_x *= aspectRatio;
	float scr_y = (pixel_coords.y*2.0 - img_size.y)/(2.0*img_size.y);

	vec3 final_color = vec3(0);
	int focus = 0, x = 0, y = 0;
	int grid = 1;
	while(grid*grid < u_NumOfSamples)
		grid++;
	
	for (int samples_processed = 0; samples_processed < u_NumOfSamples; samples_processed++){
		vec3 ray_orig = u_CameraPosn;
		vec3 ray_dirn;
		{
			vec3 cam_right = cross (u_CameraDirn, world_up);
			vec3 cam_up = cross (cam_right, u_CameraDirn);
			ivec2 process_indexs;
			if(focus < grid) {
				if(x == 0 && y == 0){
					focus++;
					x = focus, y = focus, process_indexs = ivec2(focus,focus);;
				}else{
					if(x < y) y--, process_indexs = ivec2(focus, y);
					else x--, process_indexs = ivec2(x, focus);
				}
			}else return vec4 (final_color/samples_processed, 1.0);

			float del_scr_x = aspectRatio/float(img_size.x*grid);
			float del_scr_y = 1.0/float(img_size.y*grid);
			ray_dirn = normalize(u_CameraDirn*u_FocusDist + cam_right*(scr_x + (del_scr_x*process_indexs.x)) + cam_up*(scr_y + (del_scr_y*process_indexs.y)));
		}
// C	olor
		float min_t_depth = 32000; // decrease to 50 on ray bounce
		vec3 normal_to_surface = u_CameraDirn;
		vec3 final_sample_color = vec3(0); 
		for(int i = 0; i < u_NumOfBounce; i++){

			vec3 sample_color;{ // background
				float t = (ray_dirn.y + 1.0)*0.5;
				sample_color = ((1.0 - t)*vec3 (1.0, 1.0, 1.0) + t*vec3 (0.3, 0.4, 1.0));
			}
			vec3 position, scale, obj_color;
			mat3 matrix;
			mat3 rot_matrix_of_Intersected_obj;
			int Type;

//De	pth Testing ray with objects
			const int id_tex_size_x = textureSize(u_ObjGroupIDTexture, 0).x;
			const ivec2 data_tex_size = textureSize(u_ObjGroupDataTexture, 0);
			for(int j = 0; j < u_NumOfObj; j++){
				Type = int(texture(u_ObjGroupIDTexture, vec2((j+0.1)/id_tex_size_x, 0.1)).r);					   // texture fetch is expensive
				position  = texture(u_ObjGroupDataTexture, vec2(0.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
				matrix[0] = texture(u_ObjGroupDataTexture, vec2(1.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
				matrix[1] = texture(u_ObjGroupDataTexture, vec2(2.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
				matrix[2] = texture(u_ObjGroupDataTexture, vec2(3.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
				scale     = texture(u_ObjGroupDataTexture, vec2(4.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive
				obj_color = texture(u_ObjGroupDataTexture, vec2(5.1/data_tex_size.x, (j+0.1)/data_tex_size.y)).xyz;// texture fetch is expensive

				// Although these points are transformed, they can still be directly used to 
				vec3 transformed_ray_orig = matrix*(ray_orig - position);
				vec3 transformed_ray_dirn = matrix*ray_dirn;

				Ray transformed_ray = Ray(transformed_ray_orig, normalize(transformed_ray_dirn)); // insure dirn is normalized

				float t = t_RayXObj(transformed_ray, Type, scale);
				if(min_t_depth > t && t > 0){
					normal_to_surface = SurfaceNormal(t, transformed_ray, Type, scale);
					sample_color = obj_color;
					rot_matrix_of_Intersected_obj = matrix;
					min_t_depth = t;
				}
				
			}
			final_sample_color += (sample_color*pow(0.4,i));
			if(u_ShowNormal){
				final_sample_color = normal_to_surface;
				break; // breaking out of bounce loop
			}
			if(min_t_depth > 30000){
				break; // breaking out of bounce loop
			}
			// Bounce Ray
			ray_orig = ray_orig + (min_t_depth - 0.00005)*ray_dirn; // removing just a tinybit to ensure point is above the surface instead of being inside the shape
			ray_dirn = reflect(ray_dirn, normalize(inverse(rot_matrix_of_Intersected_obj)*normal_to_surface));
			normal_to_surface = vec3(0);
			min_t_depth = 32000;
		}
		final_color += final_sample_color;

	}
	return vec4 (final_color/u_NumOfSamples, 1.0);
}
void main ()
{
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2 (gl_WorkGroupID.xy); // layout_size_x represents sampling amount

	// base pixel color for image
	vec4 pixel = out_Pixel (pixel_coords);

	// output to a specific pixel in the image
	imageStore (img_output, pixel_coords, pixel);
};