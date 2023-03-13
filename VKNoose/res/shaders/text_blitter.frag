//glsl version 4.5
#version 450

//shader input
layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in flat int textureIndex;
//output write
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform  SceneData{   
    vec4 fogColor; // w is for exponent
	vec4 fogDistances; //x for min, y for max, zw unused.
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D tex1;
layout(set = 3, binding = 0) uniform sampler samp;
layout(set = 3, binding = 1) uniform texture2D textures[91];


float fog_exp2(
  const float dist,
  const float density
) {
  const float LOG2 = -1.442695;
  float d = density * dist;
  return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}


void main() 
{
	outFragColor = texture(tex1,texCoord);

	//highp int index = int(tex_index_basecolor);
	
    //vec4 colA = texture(sampler2D(textures[textureIndex], samp), texCoord).rgba;

    vec4 colA = texture(sampler2D(textures[textureIndex], samp), texCoord).rgba;
   // vec4 colB = texture(sampler2D(textures[6], samp), texCoord).rgba;

	//outFragColor = mix(colA, colB, 0.5);
	//outFragColor.a = 0.05;
    //outFragColor.r = 0;

	outFragColor = colA;
}