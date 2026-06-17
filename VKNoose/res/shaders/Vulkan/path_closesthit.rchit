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
#include "push_constants.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 1) rayPayloadEXT bool isShadowed;
layout(location = 2) rayPayloadInEXT Payload payload;

layout(push_constant, scalar) uniform PushConstants { 
	ScenePushConstant scene; 
} pushConstant;

layout(buffer_reference, scalar) readonly buffer CameraDataBuffer { CameraData data; };
layout(buffer_reference, scalar) readonly buffer Lights { Light data[]; };
layout(buffer_reference, scalar) readonly buffer Instances { Instance data[]; };
layout(buffer_reference, scalar) readonly buffer Vertices { Vertex data[]; };
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Indices { uint data[]; };

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 2, binding = 7) uniform sampler2D laptop_render_texture;

// Static Descriptor set
layout(set = 3, binding = DESC_IDX_SAMPLERS)                uniform sampler samplers[];
layout(set = 3, binding = DESC_IDX_TEXTURES)                uniform texture2D textures[];
layout(set = 3, binding = DESC_IDX_SSBOS)                   readonly buffer GlobalSSBO { uint data[]; } ssbos[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA32F,  rgba32f) uniform image2D storage_images_rgba32f[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA16F,  rgba16f) uniform image2D storage_images_rgba16f[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA8,    rgba8)   uniform image2D storage_images_rgba8[];
						  
hitAttributeEXT vec2 attribs;

float rand(float co) { return fract(sin(co*(91.3458)) * 47453.5453); }
float rand(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }
float rand(vec3 co){ return rand(co.xy+rand(co.z)); }

vec3 CalculatePBR (vec3 baseColor, vec3 normal, float roughness, float metallic, float ao, vec3 worldPos, vec3 camPos, Light light, uint materialType) {
	
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
	//uint seed = uint(camera.frameIndex + worldPos.x + worldPos.y + worldPos.z + 6431);
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
	CameraDataBuffer cameraBuffer = CameraDataBuffer(pushConstant.scene.cameraDeviceAddress);
	CameraData camera = cameraBuffer.data;

	Lights lights = Lights(pushConstant.scene.lightsDeviceAddress);
	Instances instances = Instances(pushConstant.scene.instancesDeviceAddress);

    Instance instance = instances.data[gl_InstanceCustomIndexEXT];
	mat4 worldMatrix = instance.worldMatrix;
	uint vertexOffset = instance.vertexOffset;
	uint indexOffset = instance.indexOffset;
	uint materialType = instance.materialType;
    
	rayPayload.meshIndex = gl_InstanceCustomIndexEXT;     
    const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	Vertices vertices = Vertices(instance.vertexBufferAddress);
	Indices indices = Indices(instance.indexBufferAddress);

	uint index0 = indices.data[instance.indexOffset + gl_PrimitiveID * 3 + 0];
	uint index1 = indices.data[instance.indexOffset + gl_PrimitiveID * 3 + 1];
	uint index2 = indices.data[instance.indexOffset + gl_PrimitiveID * 3 + 2];

	Vertex v0 = vertices.data[instance.vertexOffset + index0];
	Vertex v1 = vertices.data[instance.vertexOffset + index1];
	Vertex v2 = vertices.data[instance.vertexOffset + index2];

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
    vec4 baseColor = texture(sampler2D(textures[instance.basecolorIndex], samplers[0]), texCoord).rgba;
    vec3 rma = texture(sampler2D(textures[instance.rmaIndex], samplers[0]), texCoord).rgb;	
	vec3 normalMap = texture(sampler2D(textures[instance.normalIndex], samplers[0]), texCoord).rgb;

	// Did the ray hit the laptop?
	if (materialType == 3) {
		baseColor = texture(laptop_render_texture,vec2(texCoord.x, texCoord.y)).rgba;
	}

	// Normals/tangents
	vec3 vnormal = normalize(mixBary(nrm0, nrm1, nrm2, barycentrics));
	vec3 normal  = normalize(vec3(vnormal * gl_WorldToObjectEXT));
	vec3 geonrm  = normalize(cross(pos1 - pos0, pos2 - pos0));
	vec3 tangent = normalize(mixBary(tng0.xyz, tng1.xyz, tng2.xyz, barycentrics));
	geonrm  = normalize(vec3(geonrm * gl_WorldToObjectEXT));
	tangent = normalize(vec3(tangent * gl_WorldToObjectEXT));
	vec3 bitangent = cross(normal, tangent);
	
	// World position
	vec4 modelSpaceHitPos = vec4(pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z, 1.0);
	vec3 worldPos = (worldMatrix * modelSpaceHitPos).xyz;

	// Flip normal/tangenets if backfacing
	if(dot(geonrm, -gl_WorldRayDirectionEXT) < 0) {
		geonrm = -geonrm;
	}
	if(dot(geonrm, normal) < 0) {
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
    vec3 camPos = camera.viewPos.rgb;
		
    rayPayload.color = vec3(0);
	rayPayload.normal = vec3(0);
	rayPayload.nextRayOrigin = worldPos;
	rayPayload.nextFactor = vec3(0);
	rayPayload.alpha = 1.0;
	//rayPayload.writeToImageStore = 1;

	rayPayload.normal = normal;

	rayPayload.nextRayOrigin = worldPos;

	// Bedroom light
	Light light0 =  lights.data[0];
	light0.color *= vec4(1,0.95,0.95,1);
	light0.color *= 0.75 * 0.5;
	vec3 directLighting = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light0, materialType);

	// Bathroom light
	Light light1 =  lights.data[1];
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
		vec3 I = normalize(worldPos.xyz - camera.viewPos.xyz);
		vec3 R = refract(I, normalize(normal.xyz), ratio);
		rayPayload.nextRayDirection = R;
		rayPayload.nextFactor = vec3(1);
	}
	// Hit was solid
	else {
		rayPayload.hitType = HIT_TYPE_SOLID;
		rayPayload.color = directLighting.rgb;
	}

	if (camera.inventoryOpen == 0) {
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
	if (rayPayload.bounce == 0 && instance.basecolorIndex == camera.wallpaperALBIndex) {
		rayPayload.writeToImageStore = 0;
	}
}
