#version 450

layout(location = 0) in vec3 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2DArray texturePalette;

void main() {
	vec4 texel = textureLod(texturePalette, fragUV, 6);
    outColor = vec4(texel.rgb, 1.0);
}