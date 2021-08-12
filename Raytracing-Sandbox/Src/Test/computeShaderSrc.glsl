﻿#version 440 core
layout(local_size_x = 1, local_size_y = 1) in; // note we are not changing Z for now
layout(r32f,    binding = 0) uniform image2D dpth_out;
layout(rgba32f, binding = 1) uniform image2D img_out;

layout(rgba32f, binding = 0) uniform sampler2D u_GeometryData;
layout(rgba32f, binding = 1) uniform sampler2D u_SceneHierarchy;
////// HELPER /////////////
vec3 Background_Color(vec3 ray_dirn) { // For sky or somthing like that
	float t = (ray_dirn.y + 1.0)*0.5;
	return ((1.0 - t)*vec3 (1.0, 1.0, 1.0) + t*vec3 (0.3, 0.4, 1.0));
}

#define PI 3.1415926538
const float PHI = (PI*(3.0-sqrt(5.0)));
vec3 deviateWithLinmit90deg(vec3 direction, vec3 wrt_normal, float tan_theta, uint sampleIDX, uint max_samples, float power) {// tan theta where theta represente scatter-angle (NOTE: scatteritivity EQUALS tan(theta))
	vec2 normalized; { // Sunflower Distribution
		if(sampleIDX == 0){
			normalized = vec2(0);
		}
		float b = round(2*sqrt(max_samples));

		float r = (sampleIDX > max_samples - b) ? tan_theta : tan_theta*sqrt((sampleIDX - 0.5)/(max_samples - (b+1)/2.0));
		
		float theta = PHI*sampleIDX;
		normalized =  vec2(r*cos(theta), r*sin(theta));
	}
	normalized.x = pow(normalized.x, power);
	normalized.y = pow(normalized.y, power);

	vec3 right = cross(direction, vec3(0,1,0));
	vec3 up    = cross(right    , direction  );

	float factor;
	{
		float r = dot(direction, wrt_normal);
		float s = dot(normalized.x*right + normalized.y*up, wrt_normal);
		factor = (r + s > 0.0f) ? 1.0 : - r/s;
	}
	//if(vec3 newDirn = direction + normalized.x*right + normalized.y*up;
	//	 dot(newDirn, wrt_normal) < 0.0f) 
	//{
	//	float r = dot(direction, wrt_normal);
	//	float s = dot(normalized.x*right + normalized.y*up, wrt_normal);
	//	factor = (0.0 - r) / s;
	//}
	
	return normalize(direction + factor*(normalized.x*right + normalized.y*up));
}
//#########################
////// CANTAINERS /////////
struct Transform_Data{
	vec3 Position;
	vec3 Scale;
	mat3 Rotation_Matrix;
};
struct Material { // Material Specifications
	vec3 Color;
	
    float RefractiveIndex, Refractivity, Reflectivity;
	// refractivity: how much of light gonna refract away
	// reflectivity: how much of light gonna reflect away
    
	float ScatteritivityRfr, ScatteritivityRfl; // scatteritivity (refracted, reflected): amount by which light gonna scatter/deviate
	
	float __padding;
};
////// Ray Related ////////
struct Ray{
	vec3 Origin;
	vec3 Direction;
};
struct RayHitData{
	vec3 HitPoint;
	vec3 NormalAtHit;

	Material MTL;
};

////// GLOBAL STACK ////////
shared vec3 sample_color[gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z]; // need to be checked if size is right

#define stack_capacity 40
struct FLT_STACK{
	float data[stack_capacity];
	uint size;
}; // use define for creating functions
FLT_STACK flt_Stack;
#define STK_PUSH(stack, val)              if(stack.size<stack_capacity){stack.data[stack.size]=float(val);stack.size++;}
#define STK_POP_TO(stack, store)          if(stack.size>0){stack.size--;store=stack.data[stack.size];}
#define STK_PUSH_RAY_DATA(stack, ray, contrib, bounced)     if(stack.size<(stack_capacity-7)){\
																stack.data[stack.size]=ray.Origin.x;stack.size++;\
																stack.data[stack.size]=ray.Origin.y;stack.size++;\
																stack.data[stack.size]=ray.Origin.z;stack.size++;\
																stack.data[stack.size]=ray.Direction.x;stack.size++;\
																stack.data[stack.size]=ray.Direction.y;stack.size++;\
																stack.data[stack.size]=ray.Direction.z;stack.size++;\
																stack.data[stack.size]=contrib;stack.size++;\
																stack.data[stack.size]=float(bounced);stack.size++;\
															}
#define STK_POP_RAY_DATA_TO(stack, ray, contrib, bounced)   if((stack.size-7)>0){\
																stack.size--;bounced=int(stack.data[stack.size]);\
																stack.size--;contrib=stack.data[stack.size];\
																stack.size--;ray.Direction.z=stack.data[stack.size];\
																stack.size--;ray.Direction.y=stack.data[stack.size];\
																stack.size--;ray.Direction.x=stack.data[stack.size];\
																stack.size--;ray.Origin.z=stack.data[stack.size];\
																stack.size--;ray.Origin.y=stack.data[stack.size];\
																stack.size--;ray.Origin.x=stack.data[stack.size];\
															}else ray.Origin=vec3(0),ray.Direction=vec3(0),contrib=0,bounced=0;
//#########################
float t_RayXGeom(Ray ray/* IN LOCAL COORDINATE SYSTEM OF GEOMETRY */, vec3 Scale){
	float t = -1;

	// For ELLIPSOIDS
	vec3 _xyz2,_xyz3;
	_xyz2 = vec3(ray.Origin.x/Scale.x, ray.Origin.y/Scale.y, ray.Origin.z/Scale.z);
	_xyz3 = vec3(ray.Direction.x/Scale.x, ray.Direction.y/Scale.y, ray.Direction.z/Scale.z);
	
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
	// -------------
	return (t > 0 ? t : -1);
}
vec3 SurfaceNormal(float t, Ray ray, vec3 scale){
	vec3 HitPoint = ray.Origin + t*ray.Direction;
	// ELLIPSOIDS
	return vec3(HitPoint.x/scale.x*scale.x, HitPoint.y/scale.y*scale.y, HitPoint.z/scale.z*scale.z);
	// ----------
}
bool TestIntersectAABB(vec3 minimum, vec3 maximum, Ray ray, float t_limiting) { // return true only if ray intersected and less than limit otherwise ignore
	float t1 = (minimum[0] - ray.Origin[0])*(1.0/ray.Direction[0]);
	float t2 = (maximum[0] - ray.Origin[0])*(1.0/ray.Direction[0]);
	
	float t_min = min(t1, t2);
	float t_max = max(t1, t2);

	for (int i = 0; i < 3; i++) {
        float _t1 = (minimum[i] - ray.Origin[i]) / ray.Direction[i];
		float _t2 = (maximum[i] - ray.Origin[i]) / ray.Direction[i];

		t1 = min(_t1, _t2);
		t2 = max(_t1, _t2);

		t_min = max(t1, t_min);
        t_max = min(t2, t_max);

        if (t_max <= t_min)
            return false;
    }
    return t_limiting > 0.0f ? t_limiting > t_min : true;
}
//////////////////////////

uniform uint u_NumOfGeometry;
const uint u_NumOfBunces = 3;
#define DEFAULT_COLOR            vec3(0,0,0)
#define DEFAULT_REFRACTIVE_INDEX 1.5
#define DEFAULT_REFRACTIVITY     0.7
#define DEFAULT_REFLECTIVITY     0.0
#define DEFAULT_SCATTER_REFRACT  0.0
#define DEFAULT_SCATTER_REFLECT  0.0


uniform struct
{
	vec3  Position;
	vec3  Direction;
	float FOV_y;
	//int   NumOfFocusPts;
	//float FocusDist[3];
	//float Aperture;
} u_Camera;
// #define ELLIPSOID 1; // Right now only ellipsoid

bool IntersectRay(Ray ray, float GeomIndex, inout Transform_Data final_transform, inout float t_limiting, inout vec3 normal_at_hit){
	Transform_Data transform;
	{
		vec4 data;
		data = texture(u_GeometryData, vec2(0.1 / textureSize(u_GeometryData, 0).x, (GeomIndex + 0.1) / textureSize(u_GeometryData, 0).y));
		transform.Position = data.xyz;
		transform.Rotation_Matrix[0].x = data.w;

		data = texture(u_GeometryData, vec2(1.1 / textureSize(u_GeometryData, 0).x, (GeomIndex + 0.1) / textureSize(u_GeometryData, 0).y));
		transform.Rotation_Matrix[0].y = data.x, transform.Rotation_Matrix[0].z = data.y;
		transform.Rotation_Matrix[1].x = data.z, transform.Rotation_Matrix[1].y = data.w;

		data = texture(u_GeometryData, vec2(2.1 / textureSize(u_GeometryData, 0).x, (GeomIndex + 0.1) / textureSize(u_GeometryData, 0).y));
		transform.Rotation_Matrix[1].z = data.x;
		transform.Rotation_Matrix[2] = data.yzw;

		data = texture(u_GeometryData, vec2(3.1 / textureSize(u_GeometryData, 0).x, (GeomIndex + 0.1) / textureSize(u_GeometryData, 0).y));
		transform.Scale = data.xyz;
		
		// type = int(data.w);
	}
	transform.Rotation_Matrix = transpose(transform.Rotation_Matrix); // Invert Orthogonal Mtrix
	Ray transformed = Ray(transform.Rotation_Matrix*(ray.Origin - transform.Position), transform.Rotation_Matrix*ray.Direction);
	transform.Rotation_Matrix = transpose(transform.Rotation_Matrix); // Revert back
	float t = t_RayXGeom(transformed, transform.Scale);
	if(t > 0 && t < t_limiting){
		t_limiting = t;
		final_transform = transform;
		normal_at_hit = transform.Rotation_Matrix * SurfaceNormal(t, transformed, transform.Scale);
		return true;
	}
	return false;
}

// Returns true if No further pracessing on node is needed
bool IfInsideAABBAndLeaf_TryAccumulateRI(float NodeIndex, vec3 hitPoint, inout float accumulator, inout uint accumulator_counter, out float leftChildIDX){
	vec3 BB_min, BB_max;
	float leftData, parent;
	{vec4 data;
	data = texture(u_SceneHierarchy, vec2(0.1/textureSize(u_SceneHierarchy, 0).x, (NodeIndex + 0.1)/textureSize(u_SceneHierarchy, 0).y));
	BB_min = data.xyz;
	BB_max.x = data.w;
	data = texture(u_SceneHierarchy, vec2(1.1/textureSize(u_SceneHierarchy, 0).x, (NodeIndex + 0.1)/textureSize(u_SceneHierarchy, 0).y));
	BB_max.y = data.x, BB_max.z = data.y;
	leftData = data.z, parent = data.w;}
	leftChildIDX = -1;
	if (hitPoint.x <= BB_max.x &&
		hitPoint.y <= BB_max.y &&
		hitPoint.z <= BB_max.z &&
		hitPoint.x >= BB_min.x &&
		hitPoint.y >= BB_min.y &&
		hitPoint.z >= BB_min.z)
	{ // Accumulate
		if(leftData < 0.1){ // Leaf Node with objID ∈[0 -NumOfGeom)
			Transform_Data transform;
			{
				float GeomIndex = (-leftData);
				vec4 data;
				data = texture(u_GeometryData, vec2(0.1 / textureSize(u_GeometryData, 0).x, (GeomIndex + 0.1) / textureSize(u_GeometryData, 0).y));
				transform.Position = data.xyz;
				transform.Rotation_Matrix[0].x = data.w;

				data = texture(u_GeometryData, vec2(1.1 / textureSize(u_GeometryData, 0).x, (GeomIndex + 0.1) / textureSize(u_GeometryData, 0).y));
				transform.Rotation_Matrix[0].y = data.x, transform.Rotation_Matrix[0].z = data.y;
				transform.Rotation_Matrix[1].x = data.z, transform.Rotation_Matrix[1].y = data.w;

				data = texture(u_GeometryData, vec2(2.1 / textureSize(u_GeometryData, 0).x, (GeomIndex + 0.1) / textureSize(u_GeometryData, 0).y));
				transform.Rotation_Matrix[1].z = data.x;
				transform.Rotation_Matrix[2] = data.yzw;

				data = texture(u_GeometryData, vec2(3.1 / textureSize(u_GeometryData, 0).x, (GeomIndex + 0.1) / textureSize(u_GeometryData, 0).y));
				transform.Scale = data.xyz;
				
				// type = int(data.w);
			}
			vec3 vector = hitPoint - transform.Position;
			vector = transpose(transform.Rotation_Matrix)*vector;
			vector.x /= transform.Scale.x;
			vector.y /= transform.Scale.y;
			vector.z /= transform.Scale.z;

			if(dot(vector, vector) <= 1.0){ // Inside, Accumulate RI
				accumulator += DEFAULT_REFRACTIVE_INDEX, accumulator_counter++;
				// leaf Accumulation
			}
			return true; // move Up the tree
		}
		leftChildIDX = leftData;
		return false; // move down the tree
	}
	return true; // move Up the tree
}


// Dosen't sets Normal though, Normal is set by IntersectRay() {because its faster that way}
void FillHitData(Ray ray, float t_dist, float Geom, inout RayHitData data){
	data.HitPoint = ray.Origin + t_dist*ray.Direction;
	// data.NormalAtHit = transform.Rotation_Matrix * normal;

	data.MTL.Color             = DEFAULT_COLOR;
	data.MTL.RefractiveIndex   = DEFAULT_REFRACTIVE_INDEX;
	data.MTL.Refractivity      = DEFAULT_REFRACTIVITY;
	data.MTL.Reflectivity      = DEFAULT_REFLECTIVITY;
	data.MTL.ScatteritivityRfr = DEFAULT_SCATTER_REFRACT;
	data.MTL.ScatteritivityRfl = DEFAULT_SCATTER_REFLECT;
}

#define MAX_T_DEPTH 32000.0f 
void out_Pixel(uvec2 pixel_coords, out vec3 final_color, out float final_depth){
	{
		Ray cam_ray;
		
		float AspectRatio = float(imageSize(img_out).x)/float(imageSize(img_out).y);
		float screen_dist = 1.0 / (2.0*tan(u_Camera.FOV_y*0.5));
		
		vec2 screen_rel_posn = vec2(float(pixel_coords.x)/float(imageSize(img_out).x) - 0.5, float(pixel_coords.y)/float(imageSize(img_out).y) - 0.5);
		screen_rel_posn.x *= AspectRatio;

		const vec3 world_up = vec3(0, 1, 0);
		vec3 cam_right = cross (u_Camera.Direction, world_up          );
		vec3 cam_up    = cross (cam_right         , u_Camera.Direction);

		cam_ray.Origin = u_Camera.Position + sample_color[gl_LocalInvocationIndex]; // multi-sampling
		cam_ray.Direction = normalize(u_Camera.Direction*screen_dist + cam_right*screen_rel_posn.x + cam_up*screen_rel_posn.y + sample_color[gl_LocalInvocationIndex]);

		STK_PUSH_RAY_DATA(flt_Stack, cam_ray, 1, 0);
	}

	// Test Intersection
	while (flt_Stack.size > 0)
	{
		RayHitData hitData; 
		float contribution, bounced, Surrounding_RI = 0; // Note: Surrounding_RI refers to RefractiveIndex of surrounding/Obj Encompassing Interface Object
		vec3 IncomingDirn; 
		{
			Ray currRay;
			STK_POP_RAY_DATA_TO(flt_Stack, currRay, contribution, bounced);
			
			float t_limiting = MAX_T_DEPTH; float final_GeomIndex = 0;
			
			{
				Transform_Data final_transform; 
				float final_parent, final_leaf; // Parent Node in scene heirarchy
				uint InitialSize = flt_Stack.size;
				STK_PUSH(flt_Stack, 0); // root node 
				do{
				// Find Candidate
					float GeomIndex = -1;
					float parentIDX;
					float NodeIndex;
					while(flt_Stack.size > InitialSize){
						STK_POP_TO(flt_Stack, NodeIndex);
						vec3 BB_min, BB_max;
						float leftData;
						{
							vec4 data;
							data = texture(u_SceneHierarchy, vec2(0.1/textureSize(u_SceneHierarchy, 0).x, (NodeIndex + 0.1)/textureSize(u_SceneHierarchy, 0).y));
							BB_min = data.xyz;
							BB_max.x = data.w;
							data = texture(u_SceneHierarchy, vec2(1.1/textureSize(u_SceneHierarchy, 0).x, (NodeIndex + 0.1)/textureSize(u_SceneHierarchy, 0).y));
							BB_max.y = data.x, BB_max.z = data.y;
							leftData = data.z, parentIDX = data.w;
						}
						if(TestIntersectAABB(BB_min, BB_max, currRay, t_limiting)){
							if(leftData > 0.1){ // Both are 0 or both are not
								bool invert = dot(u_Camera.Direction, vec3(1)) > 0;
								float right = leftData + 1.0;
								STK_PUSH(flt_Stack, invert ? right : leftData);
								STK_PUSH(flt_Stack, invert ? leftData : right);
							} else { // leaf
								GeomIndex = -leftData; // all else is padding
								break;
							}
						}
					}
					if(GeomIndex > -0.9) {
						bool intersected = IntersectRay(currRay, GeomIndex, final_transform, t_limiting, hitData.NormalAtHit);
						final_GeomIndex = intersected ? GeomIndex : final_GeomIndex
						, final_parent = intersected ? parentIDX : final_parent
						, final_leaf = intersected ? NodeIndex : final_leaf;
					} else break;
				} while(true);
				flt_Stack.size = InitialSize;
				IncomingDirn = currRay.Direction;
				
			}
			
			if(t_limiting < MAX_T_DEPTH) { // Some-one hit, gen_data

				FillHitData(currRay, t_limiting, final_GeomIndex, hitData);
				
				uint InitialSize = flt_Stack.size;
				// Calculate Surrounding_RI, this top-down traversal is expensive, i spent way to much time on it
				STK_PUSH(flt_Stack, 0);
				vec3 hitPT = hitData.HitPoint + 0.001*hitData.NormalAtHit;
				uint Surrounding_RI_accumulator_counter = 0;
				while(flt_Stack.size > InitialSize){
					float NodeIDX, leftChild;
					STK_POP_TO(flt_Stack, NodeIDX);
					if(!IfInsideAABBAndLeaf_TryAccumulateRI(NodeIDX, hitPT, Surrounding_RI, Surrounding_RI_accumulator_counter, leftChild)) {
						STK_PUSH(flt_Stack, leftChild);
						STK_PUSH(flt_Stack, leftChild + 1.0);
					}
				}
				//
				if(Surrounding_RI > 1.0)
					Surrounding_RI /= Surrounding_RI_accumulator_counter;
				else Surrounding_RI = 1;

			}else {
				final_color += contribution*vec3(Background_Color(currRay.Direction)), final_depth = t_limiting;
				continue;
//  ^--------This LOOP (Outer most)
			}
		}
		// Handle Hit And push rays
		if((hitData.MTL.Reflectivity > 0.002 || hitData.MTL.Refractivity > 0.002) && contribution > 0.01 && bounced < u_NumOfBunces){
			vec3 reflectedDirn = vec3(0), refractedDirn = vec3(0);
			{
				bool InnerHit = dot(hitData.NormalAtHit, IncomingDirn) > 0;
				bool Reflect = hitData.MTL.Reflectivity > 0.002f;
				bool Refract = hitData.MTL.Refractivity > 0.002f;
				if(!InnerHit) { // Apply Scatter
					if(hitData.MTL.Reflectivity > 0.002f) {
						reflectedDirn = normalize(reflect(IncomingDirn, hitData.NormalAtHit));
						if (hitData.MTL.ScatteritivityRfl > 0.001f) { //
							reflectedDirn = deviateWithLinmit90deg(reflectedDirn, hitData.NormalAtHit, hitData.MTL.ScatteritivityRfl, gl_LocalInvocationIndex, gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z, 1);
						}											  //
					}
					if(hitData.MTL.Refractivity > 0.002f) {
						refractedDirn = normalize(refract(IncomingDirn, hitData.NormalAtHit, Surrounding_RI / hitData.MTL.RefractiveIndex)); // sourceIOR/destIOR);
						if (hitData.MTL.ScatteritivityRfr > 0.001f) { //
							refractedDirn = deviateWithLinmit90deg(refractedDirn, hitData.NormalAtHit, hitData.MTL.ScatteritivityRfr, gl_LocalInvocationIndex, gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z, 1);
						}											  //
					}
				}
				else { // 100 % with no scattering
					hitData.NormalAtHit *= -1; // flip normal
					refractedDirn = refract(IncomingDirn, hitData.NormalAtHit, hitData.MTL.RefractiveIndex / Surrounding_RI); // sourceIOR/destIOR);
					if(dot(refractedDirn, refractedDirn) < 0.1f)
						reflectedDirn = reflect(IncomingDirn, hitData.NormalAtHit);
				}
			}
			float factorOfLightCarriedForward = 0.0;
			bounced++;
			if(dot(refractedDirn, refractedDirn) > 0.1f){ // push refraction-ray
				Ray newRay = Ray(hitData.HitPoint + 0.0001*hitData.NormalAtHit, refractedDirn);
				float contrib = contribution*hitData.MTL.Refractivity;
				factorOfLightCarriedForward += hitData.MTL.Refractivity;
				STK_PUSH_RAY_DATA(flt_Stack, newRay, contrib, bounced);
			}
			if(dot(reflectedDirn, reflectedDirn) > 0.1f){ // push reflection-ray
				Ray newRay = Ray(hitData.HitPoint + 0.0001*hitData.NormalAtHit, reflectedDirn);
				float contrib = contribution*hitData.MTL.Reflectivity;
				factorOfLightCarriedForward += hitData.MTL.Reflectivity;
				STK_PUSH_RAY_DATA(flt_Stack, newRay, contrib, bounced);
			}
			contribution *= (1.0 - 0.5*factorOfLightCarriedForward);
		}
		final_color += contribution*hitData.MTL.Color;
	}
	// END
}

void prepare(){
	if(gl_LocalInvocationIndex == 0){
		uint grid = 1, NumOfSamples = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
		while(grid*grid < NumOfSamples) // only 2D
			grid++;
		uint focus = 0, x = 0, y = 0;
		ivec2 sample_indexs;
		float del = 1/float(imageSize(img_out).y);
		del /= grid;
		
		for(uint i = 0; i < NumOfSamples && focus < grid; i++) {
			if(x == 0 && y == 0){
				focus++;
				x = focus, y = focus, sample_indexs = ivec2(focus,focus);
			}else{
				if(x < y) y--, sample_indexs = ivec2(focus, y);
				else x--, sample_indexs = ivec2(x, focus);
			}
			sample_color[i] = vec3(sample_indexs.x*del, sample_indexs.y*del, 0);
		}
	}
	flt_Stack.size = 0;
	flt_Stack.data[0] = 0;
}
void End(){
#if 0
	uint steps = 1, NumOfSamples = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
	while (NumOfSamples != 0)
		steps++, NumOfSamples = (NumOfSamples >> 1);
	NumOfSamples = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
	
	for(uint i = 0; i < steps; i++){
		if((gl_LocalInvocationIndex & (1 << i)) != 0) // return odd ones among left threads
			return;
		uint add_this = gl_LocalInvocationIndex + (1 << i);
		if(add_this < NumOfSamples)
			sample_color[gl_LocalInvocationIndex] += sample_color[add_this];
		barrier();
		memoryBarrierShared();
	}
	// last one thread with gl_LocalInvocationIndex == 0
	imageStore(img_out, ivec2(gl_WorkGroupID.xy), vec4(sample_color[0]/NumOfSamples, 1.0));
#else
	if(gl_LocalInvocationIndex != 0)
		return;
	uint NumOfSamples = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
	for(uint i = 1; i < NumOfSamples; i++)
		sample_color[0] += sample_color[i];
	sample_color[0] /= NumOfSamples;
	// Atlast
	imageStore(img_out, ivec2(gl_WorkGroupID.xy), vec4(sample_color[0], 1));
#endif
}
void main() {
  prepare();
  barrier();
  memoryBarrierShared();

  // get index in global work group i.e x,y position
  {
	uvec2 pixel_coords = gl_WorkGroupID.xy;
	vec3 pixel; float depth;

	// base pixel colour for image
	out_Pixel(pixel_coords, pixel, depth);

	if(gl_LocalInvocationIndex == ((gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z)/2)) // mid one
		imageStore(dpth_out, ivec2(pixel_coords), vec4(depth,0,0,0));
	
	sample_color[gl_LocalInvocationIndex] = pixel;
  }
  // output to a specific pixel in the image
  barrier();
  memoryBarrierShared();
  End();
}