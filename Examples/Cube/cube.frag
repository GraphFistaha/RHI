#version 450

layout(location = 0) in vec3 fragN;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragUV, 0, 1.0);
}