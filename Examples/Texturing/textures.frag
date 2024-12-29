#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout( push_constant ) uniform constants
{
    vec2 scale;
    vec2 pos;
	uint texture_index;
} PushConstants;

void main() {
	vec4 texel = texture(texSampler, fragUV);
    outColor = vec4(texel.rgb, 1.0);
}