#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#extension GL_EXT_scalar_block_layout : enable
//#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
//#extension GL_EXT_buffer_reference2 : require

//#extension GL_NV_ray_tracing : require
//#extension GL_EXT_nonuniform_qualifier : enable

//#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_nonuniform_qualifier : enable
//#extension GL_EXT_scalar_block_layout : enable
//#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
//#extension GL_ARB_gpu_shader_int64 : enable
//#extension GL_EXT_buffer_reference2 : require

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
//layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;

struct Vertex {
  vec3 position;
  float pad;

  vec3 normal;
  float pad2;

  vec2 texCoord;
  vec2 dummy;
};

struct ObjDesc {
	int textureIndex;
	int vertexOffset;
	int indexOffset;
	int dummy;
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

//layout(set = 2, binding = 0, scalar) buffer ObjDesc_ { 
//    ObjDesc i[]; 
//} objDesc;

hitAttributeEXT vec2 attribs;

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

void main()
{
    ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];

   	int textureIndex = objResource.textureIndex;
	int vertexOffset = objResource.vertexOffset;
	int indexOffset = objResource.indexOffset;

    const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    uint index0 = indices.i[3 * gl_PrimitiveID + indexOffset];
    uint index1 = indices.i[3 * gl_PrimitiveID + 1 + indexOffset];
    uint index2 = indices.i[3 * gl_PrimitiveID + 2 + indexOffset];

    Vertex v0 = vertices.v[index0 + vertexOffset];
    Vertex v1 = vertices.v[index1 + vertexOffset];
    Vertex v2 = vertices.v[index2 + vertexOffset];

    vec3 normal = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
    vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

    vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    vec3 camPos = cam.viewPos.rgb;

    float doom = calculateDoomFactor(hitPos, camPos, 1.0);

    vec3 fogColor = vec3(0.5);
    float fogDensity = 0.75;
    float fogDistance = distance(hitPos, camPos) * 0.12;
    

    vec3 FOG = calculateFog(hitPos, fogColor, fogDensity, fogDistance);
  

    texCoord.y = texCoord.y;
    
    vec4 outFragColor = texture(sampler2D(textures[textureIndex], samp), texCoord).rgba;
   // outFragColor = textureLod(sampler2D(textures[textureIndex], samp), texCoord, 10).rgba;

   
   

  //vec3 ambient = outFragColor.rgb;
  
    vec3 lightPos = vec3(1, 2, -1);
    vec3 FragPos = hitPos;
    vec3 viewPos = camPos;
    vec3 objectColor =  outFragColor.rgb;

    vec3 lightColor = vec3(1, 0.95, 0.8);
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
  	
    // diffuse 
    vec3 norm = normalize(normal.rgb);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // specular
    float specularStrength = 1.5;
   // specularStrength = 2.5;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular) * objectColor;
    outFragColor = vec4(result, 1.0);
    outFragColor = vec4(result, 1.0);




    
  vec3 finalColor = outFragColor.rgb * doom * doom;

  float dist = distance(hitPos * 3, vec3(0,0,0));

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
    
    //finalColor = vec3(1, 0, 0);

    rayPayload.color = finalColor;
	rayPayload.distance = gl_RayTmaxEXT;
	rayPayload.normal = normal;

    vec3 test = vec3(1.0, 1.0, 1.0);

    if (gl_InstanceCustomIndexEXT == 1) {
        finalColor = test;
    }


    rayPayload.reflector = ((finalColor.r == 1.0f) && (finalColor.g == 1.0f) && (finalColor.b == 1.0f)) ? 1.0f : 0.0f; 
}
