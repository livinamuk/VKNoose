#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;

struct Vertex {
  vec3 position;
  vec3 normal;
  vec2 uv;
};

//layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
//layout(buffer_reference, scalar) buffer Indices {int i[]; }; // Triangle indices

struct ObjDesc {
  int txtOffset;             // Texture index offset in the array of textures
  int vertexAddress;         // Address of the Vertex buffer
  int indexAddress;          // Address of the index buffer
};

layout(set = 0, binding = 2) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 viewPos;
} cam;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[92];
layout(set = 2, binding = 0, scalar) buffer Vertices { Vertex v[]; } vertices;
//layout(set = 2, binding = 0, scalar) buffer ObjDesc_ { ObjDesc i[]; } objDesc;
//layout(set = 1, binding = 2) uniform vec3 vertices[];

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
    // Object data
    //ObjDesc    objResource = objDesc.i[gl_InstanceCustomIndexEXT];
    //Indices    indices     = Indices(objResource.indexAddress);
    //Vertices   vertices    = Vertices(objResource.vertexAddress);


    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    hitValue = barycentricCoords;

    vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;


    vec3 camPos = cam.viewPos.rgb;

    float doom = calculateDoomFactor(hitPos, camPos, 1.0);

    vec3 fogColor = vec3(0.5);
    float fogDensity = 0.75;
    float fogDistance = distance(hitPos, camPos) * 0.12;
    

    vec3 FOG = calculateFog(barycentricCoords, fogColor, fogDensity, fogDistance);
    
    hitValue = FOG;// * doom;
      //  hitValue = vec3(1, 1,1) * doom;

    vec2 texCoord = barycentricCoords.xy;//vec2(0.5, 0.5);
    int textureIndex = 91;
    vec4 outFragColor = texture(sampler2D(textures[textureIndex], samp), texCoord).rgba;

    //vec4 outFragColor = vec4(1, 0, 0, 1);

  hitValue = outFragColor.rgb;
}
