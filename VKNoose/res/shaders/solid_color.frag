#version 450

layout (location = 0) out vec4 outFragColor;
layout (location = 0) in vec3 Color;

void main() {
    outFragColor = vec4(Color, 1);

    if (Color == vec3(0))
        outFragColor = vec4(1,0,0,0);
}