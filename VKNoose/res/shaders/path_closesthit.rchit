#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "raycommon.glsl"
#include "pbr_functions.glsl"

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

struct Light {
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 1) uniform CameraData_ { CameraData data; } cam;

layout(set = 0, binding = 2) buffer MeshInstances { MeshInstance i[]; } meshInstances;
layout(set = 0, binding = 4) buffer Lights_ { Light i[]; } lights;

// Static
layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[256];
layout(set = 1, binding = 2) readonly buffer Vertices { Vertex v[]; } vertices;
layout(set = 1, binding = 3) readonly buffer Indices { uint i[]; } indices;
//layout(set = 1, binding = 4, rgba8) uniform image2D laptopImage; // rt output image

layout(set = 2, binding = 1) uniform sampler2D laptop_render_texture;

hitAttributeEXT vec2 attribs;

vec3 CalculatePBR (vec3 baseColor, vec3 normal, float roughness, float metallic, float ao, vec3 worldPos, vec3 camPos, Light light, int materialType) {
	
	// compute direct light	  
	float fresnelReflect = 0.5;											// this is what they used for box, 1.0 for demon
	vec3 viewDir = normalize(camPos.xyz - worldPos);    
	float lightRadiance = 8;
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
	}
	
	// to prevent fireflies
	if(rayPayload.bounce > 0) {
		radiance = clamp(radiance, 0.0, 5.0);
		radiance = clamp(radiance, 0.0, 0.275);  // you added this, was 5
	}
	
	vec3 finalColor = radiance;

	// Doom
	float doom = calculateDoomFactor(worldPos, camPos, 1.0);
	finalColor = finalColor * doom;

	// sample indirect direction	
	vec3 random = random_pcg3d(uvec3(gl_LaunchIDEXT.xy, rayPayload.bounce));
	vec3 nextFactor = vec3(0.0);
	vec3 nextDir = sampleMicrofacetBRDF(viewDir, normal, baseColor, metallic, fresnelReflect, roughness, random, nextFactor);              
	rayPayload.nextRayDirection = nextDir;
	rayPayload.nextFactor = nextFactor;

	// prepare shadow ray
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	float shadowBias = 0.001;
	vec3 shadowRayOrigin = origin;// + shadowBias * normal;
	
	vec3 lightVector = randomDirInCone(origin, light.position.xyz); // vec3 lightVector = normalize(light.position.xyz - origin);
	float tMin   = 0.001;
    float tMax   = distance(light.position.xyz, origin);
	uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsCullFrontFacingTrianglesEXT;     	
	isShadowed  = true;
	traceRayEXT(topLevelAS,	flags, 0xFF, 0, 0, 1, shadowRayOrigin, tMin, lightVector, tMax, 1);		
	if(isShadowed) {
		rayPayload.color = vec3(0.0);
		rayPayload.done = -1;
		finalColor = vec3(0);
	}
	
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









void main()
{
    MeshInstance meshInstance = meshInstances.i[gl_InstanceCustomIndexEXT];

	int vertexOffset = meshInstance.vertexOffset;
	int indexOffset = meshInstance.indexOffset;
	mat4 worldMatrix = meshInstance.worldMatrix;
	int materialType = meshInstance.materialType;
         
    const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    uint index0 = indices.i[3 * gl_PrimitiveID + indexOffset];
    uint index1 = indices.i[3 * gl_PrimitiveID + 1 + indexOffset];
    uint index2 = indices.i[3 * gl_PrimitiveID + 2 + indexOffset];

    Vertex v0 = vertices.v[index0 + vertexOffset];
    Vertex v1 = vertices.v[index1 + vertexOffset];
    Vertex v2 = vertices.v[index2 + vertexOffset];
	
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
    vec4 baseColor = texture(sampler2D(textures[meshInstance.basecolorIndex], samp), texCoord).rgba;

	if (materialType == 3) {
		baseColor = texture(laptop_render_texture,vec2(texCoord.x, texCoord.y)).rgba; // makes no fucking difference
	}

    vec3 rma = texture(sampler2D(textures[meshInstance.rmaIndex], samp), texCoord).rgb;	
	vec3 normalMap = texture(sampler2D(textures[meshInstance.normalIndex], samp), texCoord).rgb;

	// Normal
	vec3 vnormal = normalize(mixBary(nrm0, nrm1, nrm2, barycentrics));
	vec3 normal    = normalize(vec3(vnormal * gl_WorldToObjectEXT));
	vec3 geonrm = normalize(cross(pos1 - pos0, pos2 - pos0));
	geonrm = normalize(vec3(geonrm * gl_WorldToObjectEXT));
	vec3 tangent = normalize(mixBary(tng0.xyz, tng1.xyz, tng2.xyz, barycentrics));
	tangent    = normalize(vec3(tangent * gl_WorldToObjectEXT));

	vec3 bitangent = cross(normal, tangent);
	
	mat3 tbn = mat3(normalize(bitangent), normalize(tangent), normalize(normal));
	//normal = normalize(tbn * normalize(normalMap * 2.0 - 1.0));

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
	
	float roughness = rma.r;
	float metallic = rma.g;
	float ao = rma.b;
    vec3 camPos = cam.data.viewPos.rgb;
	
  	// GLASS SKULL`
	if (gl_InstanceCustomIndexEXT == 17) {
		//roughness += 0.4;
		//roughness -= 0.4;
		//metallic = 0.91;
		//roughness = 0.01;
		//metallic += 2.9;
	}
			
	rayPayload.color = baseColor.rgb;



	//rayPayload.done = 0;
    rayPayload.color = vec3(0);
	rayPayload.distance = 0;
	rayPayload.normal = vec3(0);
	rayPayload.nextRayOrigin = worldPos;
	rayPayload.nextFactor = vec3(0);
	//rayPayload.nextRayDirection = vec3(0);
	//rayPayload.random = vec3(0);

	vec3 randomm = rayPayload.random;
	// important sampling
	float theta = asin(sqrt(randomm.y));
	float phi = 2.0 * PI * randomm.x;  
	// sampled indirect diffuse direction in normal space
	vec3 localDiffuseDir = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
	vec3 diffuseDir = getNormalSpace(normal) * localDiffuseDir;
	rayPayload.nextRayOrigin = worldPos;
	rayPayload.nextRayDirection = diffuseDir;
	//rayPayload.nextFactor = baseColor.rgb;
	rayPayload.color = baseColor.rgb;
	  





	
	rayPayload.done = 0;

	/*Light light;
	//light.position = lights[0].position.xyz;
	light.color = vec3(1, 0.95, 0.8);
	light.strength = 4.5;*/
	
	Light light0 =  lights.i[0];
	Light light1 =  lights.i[1];
	//light1.color = light0.color;
	//light0.position.xyz = vec3(0.5, 1.5, 1);
	//light1.position.xyz = vec3(-0.5, 1.5, 1);

	vec3 directLighting = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light0, materialType);

	/*Light light4;
	light4.position = vec3(-1.7376, 2, 2.85);
	light4.color =  vec3(1, 0, 0);
	light4.strength = 1.5;
	light4.radius = 10;
		
	light4.color =  vec3(1, 0.95, 0.8) * vec3(1, 0.95, 0.8) * 0.45;
	light4.strength = -0.5;
	light4.radius = 1;*/

	lights.i[1].position.z += 0.00;

	directLighting += CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light1, materialType);

//vec3	directLighting = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light4);

	rayPayload.color = directLighting.rgb / 2;


  	// RED BIG SKULL
	//if (gl_InstanceCustomIndexEXT == 7)
	//	rayPayload.color = vec3(1,0,0);
  
	// Glass (refraction)
	if (materialType == 2) {
		rayPayload.done = 3;   
		float ratio = 1.00 / 1.52;
		vec3 I = normalize(worldPos.xyz - cam.data.viewPos.xyz);
		vec3 R = refract(I, normalize(normal.xyz), ratio);
		rayPayload.nextRayDirection = R;
		
	}

	// Mirror (reflection)
	if (materialType == 1) {
		rayPayload.done = 2;     
		rayPayload.nextRayDirection = reflect(gl_WorldRayDirectionEXT, normal);
		rayPayload.nextFactor = vec3(directLighting.rgb);
		rayPayload.nextFactor = vec3(1);
	}

	// Transparent fragment
	if (baseColor.a < 0.5 ) {
		rayPayload.done = 1;
	}

	// Disable output of walls to the final image if in inventory
	if (meshInstance.basecolorIndex == cam.data.wallpaperALBIndex) {
		rayPayload.writeToImageStore = 0;
	}

}