#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "raycommon.glsl"
#include "pbr_functions.glsl"
#include "types.glsl"
#include "constants.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 1) rayPayloadEXT bool isShadowed;
layout(location = 2) rayPayloadInEXT Payload payload;

struct Vertex {
	vec3 position;
	float pad;
	vec3 normal;
	float pad2;
	vec2 texCoord;
	vec2 pad3;
	vec3 tangent;
	float pad4;
};

//TODO:
struct SceneData {
	int lightCount;
};

struct SceneDeviceAddresses {
    uint64_t vertices;
    uint64_t indices;
    uint64_t instances;
    uint64_t lights;
    uint64_t tlas;
    uint64_t sceneData;
};
// 





struct Light {
	vec4 position;
	vec4 color;
};

struct MeshInstance {
	mat4 worldMatrix;
	int vertexOffset;
	int indexOffset;
	int basecolorIndex;
	int normalIndex;
	int rmaIndex;
	int materialType; // 0 standard, 1 mirror, 2 glass 
	int dummy1;
	int dummy2;
};

layout(buffer_reference, scalar) readonly buffer LightBuffer { Light arr[]; };
layout(buffer_reference, scalar) readonly buffer SceneInstancesBuffer { MeshInstance arr[]; };

struct DeviceAddresses {
    uint64_t sceneCameraData;
    uint64_t sceneInstances;
    uint64_t sceneLights;
    uint64_t inventoryCameraData;
    uint64_t inventoryInstances;
    uint64_t inventoryLights;
    uint64_t uiInstances;
};



layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 1) uniform CameraData_ { CameraData data; } cam;

layout(set = 0, binding = 2) buffer MeshInstances { MeshInstance i[]; } meshInstances;
layout(set = 0, binding = 4) buffer Lights_ { Light i[]; } lights;

// Old Static
//layout(set = 1, binding = 2) readonly buffer Vertices { Vertex v[]; } vertices;
//layout(set = 1, binding = 3) readonly buffer Indices { uint i[]; } indices;

layout(set = 2, binding = 7) uniform sampler2D laptop_render_texture;


// Global Device addresses (TODO: split these into SceneDeviceAddresses)
layout(set = 4, binding = 0) readonly buffer GlobalAddressTable { DeviceAddresses addresses; } g_table;



// Static Descriptor set
layout(set = 3, binding = DESC_IDX_SAMLPLERS)               uniform sampler g_samplers[];
layout(set = 3, binding = DESC_IDX_TEXTURES)                uniform texture2D g_textures[];
layout(set = 3, binding = DESC_IDX_SSBOS)                   readonly buffer GlobalSSBO { uint data[]; } g_ssbos[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA32F,  rgba32f) uniform image2D g_storage_images_rgba32f[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA16F,  rgba16f) uniform image2D g_storage_images_rgba16f[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA8,    rgba8)   uniform image2D g_storage_images_rgba8[];
						  
layout(set = 3, binding = DESC_IDX_VERTICES) readonly buffer Vertices { Vertex v[]; } g_vertices; // Geometry Data (remove me when you can)
layout(set = 3, binding = DESC_IDX_INDICES)  readonly buffer Indices { uint i[]; } g_indices;     // Geometry Data (remove me when you can)





// Storage Images
//layout(set = 3, binding = 4) uniform image2D g_rtOutput;
//layout(set = 3, binding = 5) readonly buffer MousePick { uint instance; uint primitive; } g_mousePick; // MOVE ME!
//layout(set = 3, binding = 6) uniform image2D g_rtNormals;
//layout(set = 3, binding = 7) uniform image2D g_rtBaseColor;
//layout(set = 3, binding = 8) uniform image2D g_rtSecondHit;

hitAttributeEXT vec2 attribs;



float rand(float co) { return fract(sin(co*(91.3458)) * 47453.5453); }
float rand(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }
float rand(vec3 co){ return rand(co.xy+rand(co.z)); }


vec3 CalculatePBR (vec3 baseColor, vec3 normal, float roughness, float metallic, float ao, vec3 worldPos, vec3 camPos, Light light, int materialType) {
	
	// compute direct light	  
	float fresnelReflect = 0.8;											// this is what they used for box, 1.0 for demon
	vec3 viewDir = normalize(camPos.xyz - worldPos);    
	float lightRadiance = 20;
    vec3 lightDir = normalize(light.position.xyz - worldPos);           // they use something more sophisticated with a sphere
	float lightDist = max(length(light.position.xyz - worldPos), 0.1);
	float lightAttenuation = 1.0 / (lightDist*lightDist);
	lightAttenuation = clamp(lightAttenuation, 0, 1.0);					// THIS IS WRONG, but does stop super bright region around light source and doesn't seem to affect anything else...
	float irradiance = max(dot(lightDir, normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance ;
		
	vec3 radiance = vec3(0.0);
	vec3 specularContribution = vec3(0);

	// if receives light
	if(irradiance > 0.0) { 
		vec3 brdf = microfacetBRDF(lightDir, viewDir, normal, baseColor, metallic, fresnelReflect, roughness, specularContribution);
		radiance += brdf * irradiance * light.color.xyz; // diffuse shading
	//	radiance = irradiance * light.color.xyz; // diffuse shading
	}
	
	// to prevent fireflies
	if(rayPayload.bounce > 0) {
		//radiance = clamp(radiance, 0.0, 5.0);
	//	radiance = clamp(radiance, 0.0, 0.275);  // you added this, was 5
	radiance = clamp(radiance, 0.0, 0.1275);  // you added this, was 5

		//radiance = clamp(radiance, 0.025, 0.275);
	}
	
	vec3 finalColor = radiance;

	// Doom
	float doom = calculateDoomFactor(worldPos, camPos, 1.0);
	finalColor = finalColor * doom;

	// sample indirect direction	
	//uint seed = uint(cam.data.frameIndex + worldPos.x + worldPos.y + worldPos.z + 6431);
	///vec3 random = random_pcg3d(uvec3(gl_LaunchIDEXT.xy, rayPayload.bounce + seed * 6341));

	
	vec3 random;
	//rayPayload.seed += uint(random.x * 420);
	random = random_pcg3d(uvec3(gl_LaunchIDEXT.xy, rayPayload.bounce + rayPayload.seed * 6341));
	
	vec3 nextFactor = vec3(0.0);
	vec3 nextDir = sampleMicrofacetBRDF(viewDir, normal, baseColor, metallic, fresnelReflect, roughness, random, nextFactor);  
	
	//rayPayload.nextRayDirection = nextDir;
	rayPayload.nextFactor = nextFactor;

	// prepare shadow ray
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	float shadowBias = 0.000001;
	vec3 shadowRayOrigin = origin;// + shadowBias * normal;
	
	//vec3 lightVector = randomDirInCone(origin, light.position.xyz);

	shadowRayOrigin = worldPos;

	float shadowFactor = 0;
	int sampleCount = 4;
	
	for (int i = 0; i < sampleCount; i++) {

		float r = random.x;//nextRand(rayPayload.seed);
		vec3 lightVector = randomDirInCone2(origin, light.position.xyz, r, 0.05); 
		//lightVector = normalize(light.position.xyz - origin);
		float tMin   = 0.001;
		float tMax   = distance(light.position.xyz, origin);
		uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsCullFrontFacingTrianglesEXT;     	
		isShadowed  = true;
		traceRayEXT(topLevelAS,	flags, 0xFF, 0, 0, 1, shadowRayOrigin, tMin, lightVector, tMax, 1);		

		if(isShadowed) {
			shadowFactor += 1;
		}
	}
	
	shadowFactor /= sampleCount;

	rayPayload.color *= vec3(1 - shadowFactor);
	finalColor *= vec3(1 - shadowFactor);
	
	
	// Glass (refraction)
	if (materialType == 2 && !isShadowed) {
		finalColor =  specularContribution * light.color.rgb * vec3(0.5);	
		rayPayload.color = finalColor;	
	}

	// LAPTOP_DISPLAY
	if (materialType == 3) {
		finalColor = mix(finalColor, (baseColor.rgb  + specularContribution), 0.85);	
		rayPayload.color = baseColor.rgb * 0.99;
	}

	return finalColor;
}







void main() {

	
	uint64_t lightsAddress = g_table.addresses.sceneLights;
	uint64_t sceneInstancesAddress = g_table.addresses.sceneInstances;
	
    LightBuffer lights = LightBuffer(lightsAddress);
    SceneInstancesBuffer sceneInstances = SceneInstancesBuffer(sceneInstancesAddress);

	// Retreive the index of the mesh hit
    MeshInstance meshInstance = sceneInstances.arr[gl_InstanceCustomIndexEXT];
    
	//MeshInstance meshInstance = meshInstances.i[gl_InstanceCustomIndexEXT];
	rayPayload.meshIndex = gl_InstanceCustomIndexEXT;

	int vertexOffset = meshInstance.vertexOffset;
	int indexOffset = meshInstance.indexOffset;
	mat4 worldMatrix = meshInstance.worldMatrix;
	int materialType = meshInstance.materialType;
         
    const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    uint index0 = g_indices.i[3 * gl_PrimitiveID + indexOffset];
    uint index1 = g_indices.i[3 * gl_PrimitiveID + 1 + indexOffset];
    uint index2 = g_indices.i[3 * gl_PrimitiveID + 2 + indexOffset];

    Vertex v0 = g_vertices.v[index0 + vertexOffset];
    Vertex v1 = g_vertices.v[index1 + vertexOffset];
    Vertex v2 = g_vertices.v[index2 + vertexOffset];
	
	const vec3 pos0 = v0.position.xyz;
	const vec3 pos1 = v1.position.xyz;
	const vec3 pos2 = v2.position.xyz;
	const vec3 nrm0 = v0.normal.xyz;
	const vec3 nrm1 = v1.normal.xyz;
	const vec3 nrm2 = v2.normal.xyz;
	const vec2 uv0  = v0.texCoord;
	const vec2 uv1  = v1.texCoord;
	const vec2 uv2  = v2.texCoord;
	const vec4 tng0 = vec4(v0.tangent, 0);
	const vec4 tng1 = vec4(v1.tangent, 0);
	const vec4 tng2 = vec4(v2.tangent, 0);
		
    vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;
    vec4 baseColor = texture(sampler2D(g_textures[meshInstance.basecolorIndex], g_samplers[0]), texCoord).rgba;

	if (materialType == 3) {
		baseColor = texture(laptop_render_texture,vec2(texCoord.x, texCoord.y)).rgba; // makes no fucking difference
	}

    vec3 rma = texture(sampler2D(g_textures[meshInstance.rmaIndex], g_samplers[0]), texCoord).rgb;	
	vec3 normalMap = texture(sampler2D(g_textures[meshInstance.normalIndex], g_samplers[0]), texCoord).rgb;

	// Normal
	vec3 vnormal = normalize(mixBary(nrm0, nrm1, nrm2, barycentrics));
	vec3 normal    = normalize(vec3(vnormal * gl_WorldToObjectEXT));
	vec3 geonrm = normalize(cross(pos1 - pos0, pos2 - pos0));
	geonrm = normalize(vec3(geonrm * gl_WorldToObjectEXT));
	vec3 tangent = normalize(mixBary(tng0.xyz, tng1.xyz, tng2.xyz, barycentrics));
	tangent    = normalize(vec3(tangent * gl_WorldToObjectEXT));

	vec3 bitangent = cross(normal, tangent);
	


	vec4 modelSpaceHitPos = vec4(pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z, 1.0);
	vec3 worldPos = (worldMatrix * modelSpaceHitPos).xyz;

	// Adjusting normal
	const vec3 V = -gl_WorldRayDirectionEXT;
	if(dot(geonrm, V) < 0)  // Flip if back facing
		geonrm = -geonrm;

	// If backface
	if(dot(geonrm, normal) < 0) { // Make Normal and GeoNormal on the same side
		normal    = -normal;
		tangent   = -tangent;
		bitangent = -bitangent;
	}
	
	mat3 tbn = mat3(normalize(bitangent), normalize(tangent), normalize(normal));	
	rayPayload.vertexNormal = normal;	
	
	// If not glass, then sample the normal map
	// Store the vertex normal
	if (materialType != 1) {
		normal = normalize(tbn * normalize(normalMap * 2.0 - 1.0));
	}
	
	float roughness = rma.r;
	float metallic = rma.g;
	float ao = rma.b;
    vec3 camPos = cam.data.viewPos.rgb;
		
    rayPayload.color = vec3(0);
	rayPayload.normal = vec3(0);
	rayPayload.nextRayOrigin = worldPos;
	rayPayload.nextFactor = vec3(0);
	rayPayload.alpha = 1.0;
	//rayPayload.writeToImageStore = 1;

	rayPayload.normal = normal;

	rayPayload.nextRayOrigin = worldPos;


	// Bedroom light
	Light light0 =  lights.arr[0];
	light0.color *= vec4(1,0.95,0.95,1);
	light0.color *= 0.75 * 0.5;
	vec3 directLighting = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light0, materialType);

	// Bathroom light
	Light light1 =  lights.arr[1];
	light1.color *= vec4(1,0.8,0.8,1);
	light1.color *= vec4(1,0.0,0.0,1);
	light1.color *= 0.5 * 0.5;
	directLighting += CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light1, materialType);

	///////////////////////////////
	//						     //
	//	 Store the final color   //

	// Hit was transparent
	if (baseColor.a < 0.99 ) {
		rayPayload.hitType = HIT_TYPE_TRANSULUCENT;
		rayPayload.nextRayDirection = gl_WorldRayDirectionEXT;
		rayPayload.nextRayOrigin = worldPos + (gl_WorldRayDirectionEXT * 0.001);
		rayPayload.alpha =  baseColor.a;

	}
	else if (materialType == 1) {
		rayPayload.hitType = HIT_TYPE_MIRROR;
		rayPayload.nextRayDirection = reflect(gl_WorldRayDirectionEXT, normal);
		rayPayload.nextFactor = vec3(1);
		rayPayload.color = vec3(0,0,0);
	}
	// Glass
	else if (materialType == 2) {
		rayPayload.hitType = HIT_TYPE_GLASS;
		float ratio = 1.00 / 1.52;
		vec3 I = normalize(worldPos.xyz - cam.data.viewPos.xyz);
		vec3 R = refract(I, normalize(normal.xyz), ratio);
		rayPayload.nextRayDirection = R;
		rayPayload.nextFactor = vec3(1);
	}
	// Hit was solid
	else {
		rayPayload.hitType = HIT_TYPE_SOLID;
		rayPayload.color = directLighting.rgb;
	}

	if (cam.data.inventoryOpen == 0) {
		//rayPayload.color = vec3(1,0,0);
	}
  
	/*

	// little cube (test)
	if (materialType == 4) {
	
		//rayPayload.done = 1;
		rayPayload.nextRayDirection = gl_WorldRayDirectionEXT;
		rayPayload.nextRayOrigin = worldPos + (gl_WorldRayDirectionEXT * 0.0000001);
		rayPayload.alpha = 0.5;
		//rayPayload.color.xyz *= 0.5;

		//rayPayload.done = 2;     
		//rayPayload.nextRayDirection = reflect(gl_WorldRayDirectionEXT, normal);
		//rayPayload.nextFactor = vec3(directLighting.rgb);
		//rayPayload.nextFactor = vec3(1);
		//rayPayload.color = vec3(0,0,0);
	}
	*/

	// Disable output of walls to the final image if in inventory
	if (rayPayload.bounce == 0 && meshInstance.basecolorIndex == cam.data.wallpaperALBIndex) {
		rayPayload.writeToImageStore = 0;
	}
}
