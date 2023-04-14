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
const vec3 lightPosition = vec3(-0.5, 2.1, -0);
float lightStrength = 1.0;

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
    vec3 ambient = ambientStrength * light.color;  	
    vec3 norm = normalize(normal.rgb);
    vec3 lightDir = normalize(light.position - worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color * lightStrength;    
    float specularStrength = 1.5;
    vec3 viewDir = normalize(viewPos - worldPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * light.color ;        

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
	vec3 color = NdotL * light.color * (diffuseContrib + specContrib);
	vec3 diffuse = pbrInputs.diffuseColor;        

	float attenuation2  = CalculateAttenuation(light, worldPos);

	vec3 radiance = light.color * attenuation2 * 300;  
	vec3 diffuseColor2 = albedo * (vec3(1.0) - f0);
	color *= radiance;
	
    // HDR tonemapping
	//color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 


	vec3 pbr = color;
    vec3 phong = CalculatePhong(worldPos, baseColor, normal, camPos, light);
	//vec3 finalColor = mix(phong, pbr, 0.45);
	vec3 finalColor = mix(phong, pbr, 0.05);
	

	float bias = 0.0;
	//vec3 origin =  worldPos + normal * bias;	
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	vec3 lightVector = normalize(lightPos - origin);

	float tMin   = 0.001;
    float tMax   = distance(lightPos, origin);
	
	vec2 rng = vec2(0.0015);
	vec3 light_dir       = lightVector;
    vec3 light_tangent   = normalize(cross(light_dir, vec3(0.0f, 1.0f, 0.0f)));
    vec3 light_bitangent = normalize(cross(light_tangent, light_dir));
    // calculate disk point
    float point_radius = light.radius * sqrt(rng.x);
    float point_angle  = rng.y * 2.0f * PI;
    vec2  disk_point   = vec2(point_radius * cos(point_angle), point_radius * sin(point_angle));    
    vec3 Wi = normalize(light_dir + disk_point.x * light_tangent + disk_point.y * light_bitangent);



    vec3 rayDir = Wi;//
	rayDir  = lightVector;
    //vec3  origin = worldPos;// + normal * 0.015;
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
      finalColor *= 0.0;//25;
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

	vec3 bitangent = cross(normal, tangent);
	
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
	light.color = vec3(1, 0.95, 0.8);
	light.strength = 2.5;
	light.radius = 5;
	light.magic = 88;
	
	Light light2;
	light2.position = vec3(-2,1,-1.5);
	light2.color =  vec3(1, 0.95, 0.8);
	light2.strength = 3;
	light2.radius = 7;
	light2.magic = 88;

	Light light3;
	light3.position = vec3(-2,1,1.5);
	light3.color =  vec3(1, 0.95, 0.8);
	light3.strength = 3;
	light3.radius = 7;
	light3.magic = 88;

	Light light4;
	light4.position = vec3(-1.7376, 2, 2.85);
	light4.color =  vec3(1, 0, 0);
	//light4.color =  vec3(1, 0.95, 0.8);
	light4.strength = 1.5;
	light4.radius = 10;
	light4.magic = 88;

	Light light5;
	light5.position = vec3(-3.8, 2, -8.5);
	light5.color =  vec3(1, 0.95, 0.8);
	light5.strength = 20;
	light5.radius = 7;
	light5.magic = 88;

		
	//vec3 lightPosition2 = ;
	//vec3 lightPosition3 = vec3(-2,1,-1.5);
	
	vec3 finalColor1 = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light);
	vec3 finalColor2 = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light2);
	vec3 finalColor3 = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light3);
	vec3 finalColor4 = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light4);
	vec3 finalColor5 = CalculatePBR(baseColor.rgb, normal, roughness, metallic, ao, worldPos, camPos, light5);


	//finalColor = finalColor + finalColor2 + finalColor3;
    vec3 finalColor =  finalColor1 + finalColor2 + finalColor3 + finalColor4;
   finalColor =  finalColor1 + finalColor4 + finalColor5;

   finalColor += baseColor.rgb * vec3(0.0);
   // finalColor =  finalColor4;;
	
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


	
//	rayPayload.color = vec3(CalculateAttenuation(light, worldPos));


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
	

	//rayPayload.color = normal;

    if (gl_InstanceCustomIndexEXT == 2 || 
	gl_InstanceCustomIndexEXT == 3 || 
	gl_InstanceCustomIndexEXT == 6 ||
	gl_InstanceCustomIndexEXT == 7 || 
	gl_InstanceCustomIndexEXT == 8 || 
	gl_InstanceCustomIndexEXT == 9 || 
	gl_InstanceCustomIndexEXT == 10 || 
	gl_InstanceCustomIndexEXT == 11 || 
	gl_InstanceCustomIndexEXT == 12|| 
	gl_InstanceCustomIndexEXT == 13 || 
	gl_InstanceCustomIndexEXT == 14 || 
	//gl_InstanceCustomIndexEXT == 51 || // toilet seat
	//gl_InstanceCustomIndexEXT == 50 || // toilet lid`
	gl_InstanceCustomIndexEXT == 50 || // toilet 
	gl_InstanceCustomIndexEXT == 58 ||  //basin 
	gl_InstanceCustomIndexEXT == 57 ||  //basin
	gl_InstanceCustomIndexEXT == 56 ||  //basin
	gl_InstanceCustomIndexEXT == 66 ) // mirror
	{//|| metallic > 0.9|| roughness > 0.1) {
		rayPayload.done = 0;		
    }
		 
	if (gl_InstanceCustomIndexEXT == 66 ){
		
		rayPayload.done = 2;
		//rayPayload.color = vec3(0, 1,1);
    }
	//	rayPayload.color = normal;




	vec3 acc = vec3(0);
	{
	
		float bias = 0.0;
		//vec3 origin =  worldPos + normal * bias;	
		vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		vec3 lightVector = normalize(light.position - origin);

		float tMin   = 0.001;
		float tMax   = distance(light.position, origin);

		//const vec2 current_coord = vec2(gl_GlobalInvocationID.xy);
		vec2 rng = vec2(0);
		rng.x = random(vec2(worldPos.x, worldPos.y));
		rng.y = random(vec2(worldPos.y, worldPos.x));
	

		vec3 light_dir       = lightVector;
		vec3 light_tangent   = normalize(cross(light_dir, vec3(0.0f, 1.0f, 0.0f)));
		vec3 light_bitangent = normalize(cross(light_tangent, light_dir));
		// calculate disk point
		float radius = 0.05;
		float point_radius = radius * sqrt(rng.x);
		float point_angle  = rng.y * 2.0f * PI;
		vec2  disk_point   = vec2(point_radius * cos(point_angle), point_radius * sin(point_angle));    
		vec3 Wi = normalize(light_dir + disk_point.x * light_tangent + disk_point.y * light_bitangent);



		vec3 rayDir = Wi;// lightVector;
		// rayDir = lightVector;
		//vec3  origin = worldPos;// + normal * 0.015;
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
		float att = CalculateAttenuation(light, origin);

		vec3 COLOR = vec3(att * light2.color);    
		if(isShadowed)
		{
		  COLOR *= 0.0;//25;
		}
		acc += COLOR;
	}
		{
	
		float bias = 0.0;
		//vec3 origin =  worldPos + normal * bias;	
		vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		vec3 lightVector = normalize(light4.position - origin);

		float tMin   = 0.001;
		float tMax   = distance(light4.position, origin);

		//const vec2 current_coord = vec2(gl_GlobalInvocationID.xy);
		vec2 rng = vec2(0);
		rng.y = random(vec2(worldPos.x, worldPos.y));
		rng.x = random(vec2(worldPos.y, worldPos.x));
	

		vec3 light_dir       = lightVector;
		vec3 light_tangent   = normalize(cross(light_dir, vec3(0.0f, 1.0f, 0.0f)));
		vec3 light_bitangent = normalize(cross(light_tangent, light_dir));
		// calculate disk point
		float radius = 0.05;
		float point_radius = radius * sqrt(rng.x);
		float point_angle  = rng.y * 2.0f * PI;
		vec2  disk_point   = vec2(point_radius * cos(point_angle), point_radius * sin(point_angle));    
		vec3 Wi = normalize(light_dir + disk_point.x * light_tangent + disk_point.y * light_bitangent);



		vec3 rayDir = Wi;// lightVector;
		// rayDir = lightVector;
		//vec3  origin = worldPos;// + normal * 0.015;
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
		float att = CalculateAttenuation(light4, origin);
		//att = 1;
		vec3 COLOR = vec3(att * light4.color);    
		if(isShadowed)
		{
		  COLOR *= 0.0;//25;
		}
		acc += COLOR;
	}
	acc *= 1;

	//acc = acc * (baseColor.rgb * 3.5) + (baseColor.rgb * 0.5);
	//acc += vec3(baseColor.rgb * 0.5);

	//rayPayload.color = acc;

}
