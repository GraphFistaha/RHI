#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBlock {
    float t;
} ub;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
	vec4 texel = texture(texSampler, fragUV);
    outColor = vec4(texel.rgb * ub.t, 1.0);
}