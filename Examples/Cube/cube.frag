#version 450

layout(location = 0) in vec3 fragN;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in flat uint fragTextureIndex;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texturePalette[8];

void main() {
    vec4 texel = texture(texturePalette[fragTextureIndex], fragUV);
    outColor = texel;
}