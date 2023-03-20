#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 2) rayPayloadEXT bool shadowed;

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

layout(set = 0, binding = 3) buffer Vertices { Vertex v[]; } vertices;
layout(set = 0, binding = 4) buffer Indices { uint i[]; } indices;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[256];

layout(set = 2, binding = 0) buffer ObjDesc_ { ObjDesc i[]; } objDesc;

hitAttributeEXT vec2 attribs;

const float PI = 3.14159265359;  
const vec3 lightPos = vec3(-2.2, 2, -2.5);
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
        distanceFromCamera *= 0.125;
        doomFactor = (1 - distanceFromCamera);
       // doomFactor = clamp(doomFactor, 0.23, 1.0);
        doomFactor = clamp(doomFactor, 0.1, 1.0);
    }
    return doomFactor;
}



// PHONG

vec3 CalculatePhong(vec3 fragPos, vec3 baseColor, vec3 normal, vec3 camPos) {
    vec3 viewPos = camPos;
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * lightColor;  	
    vec3 norm = normalize(normal.rgb);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;    
    float specularStrength = 1.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;        
    return (ambient + diffuse + specular) * baseColor;
}


struct PBRInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting
	vec3 specularColor;           // color contribution from specular lighting
};
vec3 specularReflection(PBRInfo pbrInputs) {
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}
float geometricOcclusion(PBRInfo pbrInputs) {
	float NdotL = pbrInputs.NdotL;
	float NdotV = pbrInputs.NdotV;
	float r = pbrInputs.alphaRoughness;
	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}
float microfacetDistribution(PBRInfo pbrInputs) {
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
	return roughnessSq / (PI * f * f);
}
vec3 diffuse(PBRInfo pbrInputs) {
	return pbrInputs.diffuseColor / PI;
}
float saturate(float value) {
	return clamp(value, 0.0, 1.0);
}


vec3 mixBary(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
	return (a * barycentrics.x + b * barycentrics.y + c * barycentrics.z);
}

vec3 srgb_to_linear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
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
	
	// Normal
	vec3 normal = normalize(mixBary(nrm0, nrm1, nrm2, barycentrics));
	normal    = normalize(vec3(normal * gl_WorldToObjectEXT));
	vec3 geonrm = normalize(cross(pos1 - pos0, pos2 - pos0));
	geonrm = normalize(vec3(geonrm * gl_WorldToObjectEXT));
	vec3 tangent = normalize(mixBary(tng0.xyz, tng1.xyz, tng2.xyz, barycentrics));
	tangent    = normalize(vec3(tangent * gl_WorldToObjectEXT));
	vec3 bitangent = normalize(mixBary(btng0.xyz, btng1.xyz, btng2.xyz, barycentrics));
	bitangent    = normalize(vec3(bitangent * gl_WorldToObjectEXT));

	//normal = bitangent;
	
	  // Adjusting normal
	  const vec3 V = -gl_WorldRayDirectionEXT;
	  if(dot(geonrm, V) < 0)  // Flip if back facing
		geonrm = -geonrm;

	  // If backface
	  if(dot(geonrm, normal) < 0)  // Make Normal and GeoNormal on the same side
	  {
		normal    = -normal;
		tangent   = -tangent;
		bitangent = -bitangent;
	  }

    vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

    vec4 baseColor = texture(sampler2D(textures[objResource.basecolorIndex], samp), texCoord).rgba;
    vec3 rma = texture(sampler2D(textures[objResource.rmaIndex], samp), texCoord).rgb;
	
	vec3 normalMap = texture(sampler2D(textures[objResource.normalIndex], samp), texCoord).rgb;

	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));
	normal = normalize(tbn * normalize(normalMap * 2.0 - 1.0));

	vec3 vertexNormal = (v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
	vec3 vertexTangent = (v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z);
	vec3 vertexBiTangent = (v0.bitangent * barycentrics.x + v1.bitangent * barycentrics.y + v2.bitangent * barycentrics.z);
	
	vec3 attrNormal = mat3( transpose( inverse( worldMatrix ) ) ) * vertexNormal;
	vec3 attrTangent = mat3( transpose( inverse( worldMatrix ) ) ) * vertexTangent;
	vec3 attrBiTangent = mat3( transpose( inverse( worldMatrix ) ) ) * vertexBiTangent;
	
	attrNormal = mat3( transpose( inverse( worldMatrix ) ) ) * vertexNormal;
	attrTangent = mat3( transpose( inverse( worldMatrix ) ) ) * vertexTangent;
	attrBiTangent = mat3( transpose( inverse( worldMatrix ) ) ) * vertexBiTangent;

	tbn = mat3(normalize(attrBiTangent), normalize(attrTangent), normalize(attrNormal));
	normal = normalize(tbn * normalize(normalMap * 2.0 - 1.0));

    vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 camPos = cam.viewPos.rgb;

	
	

//	normal = normalMap * 1.22;
	//normal = vec3(0.5, 0.5, 1);


	/*vec3 tangentNormal = normalMap * 2.0 - 1.0;
    vec3 Q1  = dFdx(hitPos);
    vec3 Q2  = dFdy(hitPos);
    vec2 st1 = dFdx(texCoord);
    vec2 st2 = dFdy(texCoord);
    vec3 N   = normalize(normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    normal = normalize(TBN * tangentNormal);*/

    float doom = calculateDoomFactor(hitPos, camPos, 1.0);
    vec3 fogColor = vec3(0.5);
    float fogDensity = 0.75;
    float fogDistance = distance(hitPos, camPos) * 0.12;  
    vec3 FOG = calculateFog(hitPos, fogColor, fogDensity, fogDistance);
  
    

    vec3 result = CalculatePhong(hitPos.rgb, baseColor.rgb, normal.rgb, camPos.rgb);
    vec4 outFragColor = vec4(result, 1.0);
    
    vec3 finalColor = outFragColor.rgb * doom * doom;

    float dist = distance(hitPos * 3, vec3(0,1,0));

    float attenuation = 1.0 / (dist * dist);
    attenuation = clamp(attenuation, 0, 1);

    float doom2 = attenuation;

  
    finalColor = finalColor * doom2;


    vec3 lightVector = normalize(lightPos);
	float dot_product = max(dot(lightVector, normal), 0.2);
 // Shadow casting
	float tmin = 0.001;
	float tmax = 10000.0;
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	shadowed = true;  

    
     //   hitValue = vec3(0, 0, 0);

	// Trace shadow ray and offset indices to match shadow hit/miss shader group indices
	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 1, 0, 1, origin, tmin, lightVector, tmax, 2);
	if (shadowed) {
		finalColor *= 0.25;
       // hitValue = vec3(1, 0, 0);
    }
    
    //finalColor = normal;//vec3(1, 0, 0);

    rayPayload.color = finalColor;
	rayPayload.distance = gl_RayTmaxEXT;
	rayPayload.normal = normal;







	float lightStrength = 10.0;

     
    vec3 albedo = baseColor.rgb;
    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;
      


    vec3 worldPos = hitPos;

    // Get textures
//	baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
	vec3 RMA = rma;
    vec3 n = normal;


	float perceptualRoughness = RMA.r;
	vec3 diffuseColor;
    vec3 viewPos = camPos.xyz;

    perceptualRoughness = min(perceptualRoughness, 0.99);
    metallic = min(metallic, 0.99);
    perceptualRoughness = max(perceptualRoughness, 0.01);
    metallic = max(metallic, 0.01);
   //
    
	vec3 f0 = vec3(0.04);

	diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;

	float alphaRoughness = perceptualRoughness * perceptualRoughness;
	vec3 specularColor = mix(f0, baseColor.rgb, metallic);
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	n = normalize(n);
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
   
	//vec3 brdf = (texture(BRDF_LUT, vec2(pbrInputs.NdotV, 1.0 - metallic))).rgb;
	vec3 diffuse = pbrInputs.diffuseColor;
        
	//float strength = 100;
	float radius =1;
	float magic = 0.025;
    float dist2 = length(worldPos - lightPos);
    float num = saturate(1 - (dist2 / radius) * (dist2 / radius) * (dist2 / radius) * (dist2 / radius));
	float attenuation2 = num * num / (dist2 * dist2 + magic * 2.5);
    // attenuation
	float att = 1.0 / (1.0 + 0.1*dist2 + 0.01*dist2*dist2);
	//att *= 1.5;
	float strength = lightStrength * 0.5;
	float lumance = att * att * att * att * att * att * att  * att * strength;
	attenuation2 = min(lumance, 1);
	vec3 radiance = lightColor * attenuation2;  
	vec3 diffuseColor2 = albedo * (vec3(1.0) - f0);
	color *= radiance;
	
    // HDR tonemapping
	color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    rayPayload.color = color;













    
    if (gl_InstanceCustomIndexEXT == 1) {
        finalColor = vec3(0.0, 0.0, 0.0);
    }
    
    if (gl_InstanceCustomIndexEXT == 12) {
      //  finalColor = vec3(1.0, 1.0, 1.0);
    }

    if (gl_InstanceCustomIndexEXT == 11) {
       // finalColor = vec3(1.0, 1.0, 1.0);
    }


   //rayPayload.color = outFragColor.rgb;//vec3(texCoord, 0);
  

    rayPayload.reflector = 0;

    if ((finalColor.r == 0.0f) && (finalColor.g == 0.0f) && (finalColor.b == 0.0f))  {
         rayPayload.reflector = 1;
    }
    else if ((finalColor.r == 1.0f) && (finalColor.g == 1.0f) && (finalColor.b == 1.0f)) {
         rayPayload.reflector = 2;
         rayPayload.color = normal;//vec3(0, 1, 0);
    }

  //  rayPayload.color = normal * 0.5 + 0.5;
 //   rayPayload.color = normal;
    rayPayload.reflector = 0.5;


	
	//rayPayload.color = normal;
}
