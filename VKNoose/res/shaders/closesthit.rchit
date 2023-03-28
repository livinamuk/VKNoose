#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "raycommon.glsl"
#include "pbr_functions.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 1) rayPayloadEXT bool isShadowed;

struct Vertex {
	vec3 position;
	float pad;
	vec3 normal;
	float pad2;
	vec2 texCoord;
	vec2 pad3;
	vec3 tangent;
	float pad4;
	vec3 bitangent;
	float pad5;
};

struct ObjDesc {
	mat4 worldMatrix;
	int vertexOffset;
	int indexOffset;
	int basecolorIndex;
	int normalIndex;
	int rmaIndex;
	int dummy;
	int dummy1;
	int dummy2;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 2) uniform CameraProperties {
	mat4 viewInverse;
	mat4 projInverse;
	vec4 viewPos;
    int sizeOfVertex;
} cam;

layout(set = 0, binding = 3) readonly buffer Vertices { Vertex v[]; } vertices;
layout(set = 0, binding = 4) readonly buffer Indices { uint i[]; } indices;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[256];

layout(set = 2, binding = 0) buffer ObjDesc_ { ObjDesc i[]; } objDesc;

hitAttributeEXT vec2 attribs;

const float PI = 3.14159265359;  
//const vec3 lightPos = vec3(-2.2, 2, -3.5);
const vec3 lightPosition = vec3(0, 2, -0);
float lightStrength = 1.0;
const vec3 lightColor = vec3(1, 0.95, 0.8);

float fog_exp2(const float dist, const float density) {
  const float LOG2 = -1.442695;
  float d = density * dist;
  return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

vec3 calculateFog(vec3 inputColor, vec3 fogColor, float fogDensity, float fogDistance) {
    float fogAmount = fog_exp2(fogDistance, fogDensity);
    return mix(inputColor, fogColor, fogAmount);
}

float calculateDoomFactor(vec3 fragPos, vec3 camPos, float fallOff)
{
    float distanceFromCamera = distance(fragPos, camPos);
    float doomFactor = 1;	
    if (distanceFromCamera > fallOff) {
        distanceFromCamera -= fallOff;
        distanceFromCamera *= 0.13;
        doomFactor = (1 - distanceFromCamera);
       // doomFactor = clamp(doomFactor, 0.23, 1.0);
        doomFactor = clamp(doomFactor, 0.1, 1.0);
    }
    return doomFactor;
}


struct Light {
	vec3 position;
	vec3 color;
	float strength;
	float radius;
	float magic;
};

float CalculateAttenuation(Light light, vec3 worldPos) {
	float radius = light.radius;
	float magic = light.magic;
    float dist2 = length(worldPos - light.position);
    float num = saturate(1 - (dist2 / radius) * (dist2 / radius) * (dist2 / radius) * (dist2 / radius));
	
	float attenuation2 = num * num / (dist2 * dist2 + magic * 2.5);
    // attenuation
	float att = 1.0 / (1.0 + 0.1*dist2 + 0.01*dist2*dist2);
	//att *= 1.5;
	float strength = light.strength;
	float lumance = att * att * att * att * att * att * att  * att * strength;
	return min(lumance, 1);
}

// PHONG

vec3 CalculatePhong(vec3 worldPos, vec3 baseColor, vec3 normal, vec3 camPos, Light light) {
    vec3 viewPos = camPos;
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * lightColor;  	
    vec3 norm = normalize(normal.rgb);
    vec3 lightDir = normalize(light.position - worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * lightStrength;    
    float specularStrength = 1.5;
    vec3 viewDir = normalize(viewPos - worldPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor ;        

	float attenuation = CalculateAttenuation(light, worldPos);
    return (ambient + diffuse + specular) * baseColor * attenuation;
}


// PBR

vec3 CalculatePBR (vec3 baseColor, vec3 normal, float roughness, float metallic, float ao, vec3 worldPos, vec3 camPos, Light light) {
	
	vec3 lightPos = light.position;
	vec3 albedo = baseColor;
	float perceptualRoughness = roughness;
	vec3 diffuseColor;
    vec3 viewPos = camPos.xyz;

    perceptualRoughness = min(perceptualRoughness, 0.99);
    metallic = min(metallic, 0.99);
    perceptualRoughness = max(perceptualRoughness, 0.01);
    metallic = max(metallic, 0.01);
	vec3 f0 = vec3(0.04);
	diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;
	float alphaRoughness = perceptualRoughness * perceptualRoughness;
	vec3 specularColor = mix(f0, baseColor.rgb, metallic);
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;
	vec3 n = normalize(normal);
	vec3 v = normalize(viewPos - worldPos);    // Vector from surface point to camera
	vec3 l = normalize(lightPos - worldPos);     // Vector from surface point to light
	vec3 h = normalize(l+v);                        // Half vector between both l and v
	vec3 reflection = -normalize(reflect(v, n));
	reflection.y *= -1.0f;
	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);    

	PBRInfo pbrInputs = PBRInfo(
		NdotL,
		NdotV,
		NdotH,
		LdotH,
		VdotH,
		perceptualRoughness,
		metallic,
		specularEnvironmentR0,
		specularEnvironmentR90,
		alphaRoughness,
		diffuseColor,
		specularColor
	);
	vec3 F = specularReflection(pbrInputs);
	float G = geometricOcclusion(pbrInputs);
	float D = microfacetDistribution(pbrInputs);
	vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
	vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
	vec3 color = NdotL * lightColor * (diffuseContrib + specContrib);
	vec3 diffuse = pbrInputs.diffuseColor;        

	float attenuation2  = CalculateAttenuation(light, worldPos);

	vec3 radiance = lightColor * attenuation2;  
	vec3 diffuseColor2 = albedo * (vec3(1.0) - f0);
	color *= radiance;
	
    // HDR tonemapping
	//color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

	vec3 lightVector = normalize(lightPos - worldPos);

	vec3 pbr = color;
    vec3 phong = CalculatePhong(worldPos, baseColor, normal, camPos, light);
	//vec3 finalColor = mix(phong, pbr, 0.45);
	vec3 finalColor = mix(phong, pbr, 0.05);

	float tMin   = 0.001;
    float tMax   = length(lightPos - worldPos);;
    vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3  rayDir = lightVector;
    uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    isShadowed   = true;
    traceRayEXT(topLevelAS,  // acceleration structure
                flags,       // rayFlags
                0xFF,        // cullMask
                0,           // sbtRecordOffset
                0,           // sbtRecordStride
                1,           // missIndex
                origin,      // ray origin
                tMin,        // ray min range
                rayDir,      // ray direction
                tMax,        // ray max range
                1            // payload (location = 1)
    );
    if(isShadowed)
    {
      finalColor *= 0.25;
    }
	return finalColor;
}



void main()
{
    ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];

	int vertexOffset = objResource.vertexOffset;
	int indexOffset = objResource.indexOffset;
	mat4 worldMatrix = objResource.worldMatrix;
         
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
	const vec4 btng0 = vec4(v0.bitangent, 0);
	const vec4 btng1 = vec4(v1.bitangent, 0);
	const vec4 btng2 = vec4(v2.bitangent, 0);
		
    vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;
    vec4 baseColor = texture(sampler2D(textures[objResource.basecolorIndex], samp), texCoord).rgba;
    vec3 rma = texture(sampler2D(textures[objResource.rmaIndex], samp), texCoord).rgb;	
	vec3 normalMap = texture(sampler2D(textures[objResource.normalIndex], samp), texCoord).rgb;

	// Normal
	vec3 vnormal = normalize(mixBary(nrm0, nrm1, nrm2, barycentrics));
	vec3 normal    = normalize(vec3(vnormal * gl_WorldToObjectEXT));
	vec3 geonrm = normalize(cross(pos1 - pos0, pos2 - pos0));
	geonrm = normalize(vec3(geonrm * gl_WorldToObjectEXT));
	vec3 tangent = normalize(mixBary(tng0.xyz, tng1.xyz, tng2.xyz, barycentrics));
	tangent    = normalize(vec3(tangent * gl_WorldToObjectEXT));
	vec3 bitangent = normalize(mixBary(btng0.xyz, btng1.xyz, btng2.xyz, barycentrics));
	bitangent    = normalize(vec3(bitangent * gl_WorldToObjectEXT));
	
	mat3 tbn = mat3(normalize(bitangent), normalize(tangent), normalize(normal));
	normal = normalize(tbn * normalize(normalMap * 2.0 - 1.0));

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
    vec3 camPos = cam.viewPos.rgb;





	Light light;
	light.position = lightPosition;
	light.color = vec3(1,1,1);
	light.strength = 2.5;
	light.radius = 5;
	light.magic = 88;
	
	Light light2;
	light2.position = vec3(-2,1,-1.5);
	light2.color = vec3(1,1,1);
	light2.strength = 3;
	light2.radius = 7;
	light2.magic = 88;

	Light light3;
	light3.position = vec3(-2,1,1.5);
	light3.color = vec3(1,1,1);
	light3.strength = 3;
	light3.radius = 7;
	light3.magic = 88;

		
	//vec3 lightPosition2 = ;
	//vec3 lightPosition3 = vec3(-2,1,-1.5);
	
	vec3 finalColor1 = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light);
	vec3 finalColor2 = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light2);
	vec3 finalColor3 = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light3);


	//finalColor = finalColor + finalColor2 + finalColor3;
    vec3 finalColor =  finalColor1 + finalColor2 + finalColor3;
	
    // HDR tonemapping
	//phong = phong / (phong + vec3(1.0));
    // gamma correct
   // phong = pow(phong, vec3(1.0/2.2)); 



     
    




	
	if (baseColor.a < 0.5)  {
		finalColor = vec3(0,0,0);
    }
	//firstHitColor
		
    float doom = calculateDoomFactor(worldPos, camPos, 1.0);
    vec3 fogColor = vec3(0.5);
    float fogDensity = 0.75;
    float fogDistance = distance(worldPos, camPos) * 0.15;  
    vec3 FOG = calculateFog(finalColor, fogColor, fogDensity, fogDistance);
    //finalColor = finalColor.rgb * doom * doom;
    float dist = distance(worldPos * 3, vec3(0,1,0));
    float attenuation = 1.0 / (dist * dist);
    attenuation = clamp(attenuation, 0, 1);
    float doom2 = attenuation;  
    //finalColor = finalColor * doom2;
	

	//finalColor = FOG * doom* doom;
	//finalColor = FOG * doom* doom;
	finalColor = finalColor * doom;

//    finalColor = normal;//vec3(1, 0, 0);
	//hitPos = vec3(worldMatrix * vec4(hitPos, 1));

	//finalColor = worldPos2;
	//finalColor = vnormal;

    rayPayload.color = finalColor;
	rayPayload.distance = gl_RayTmaxEXT;
	rayPayload.normal = normal;

//rayPayload.color = vec4(1, 0, 1, 1);


	const vec3 pos      = pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z;
	const vec3 worldPosition = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0)); 

	const vec3 nrm      = nrm0 * barycentrics.x + nrm1 * barycentrics.y + nrm2 * barycentrics.z;
	const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));  

	// If first bounce, the record the hit
	if (rayPayload.done == -1) {
		rayPayload.instanceIndex = gl_InstanceCustomIndexEXT;
		rayPayload.primitiveIndex = gl_PrimitiveID;
	}

	if (rayPayload.done == 1) {
		rayPayload.color = rayPayload.color * vec3(0.3025);
	}

	rayPayload.done = 1;
	rayPayload.hitPos = worldPosition;// + (normal * vec3(0.1));
	//rayPayload.reflectDir = reflect(gl_WorldRayDirectionEXT, worldNrm);
	rayPayload.normal = normal;
	rayPayload.attenuation = vec3(1,1,1);

	//rayPayload.color = vec3(texCoord, 0);
	

    if (gl_InstanceCustomIndexEXT == 1 || 
	gl_InstanceCustomIndexEXT == 2 || 
	gl_InstanceCustomIndexEXT == 3 || 
	gl_InstanceCustomIndexEXT == 4 || 
	gl_InstanceCustomIndexEXT == 5 ){//|| metallic > 0.9|| roughness > 0.1) {
		rayPayload.done = 0;		
    }

//rayPayload.color = normal;
}
