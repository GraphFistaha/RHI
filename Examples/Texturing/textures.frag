#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texture0;
layout(binding = 1) uniform sampler2D texture1;
layout(binding = 2) uniform sampler2D texture2;
layout(binding = 3) uniform sampler2D texture3;
layout(binding = 4) uniform sampler2D texture4;
layout(binding = 5) uniform sampler2D texture5;
layout(binding = 6) uniform sampler2D texture6;
layout(binding = 7) uniform sampler2D texture7;

vec4 GetTexelById(uint id, vec2 uv)
{
    if (id == 0)
    return texture(texture0, uv);
    if (id == 1)
    return texture(texture1, uv);
    if (id == 2)
    return texture(texture2, uv);
    if (id == 3)
    return texture(texture3, uv);
    if (id == 4)
    return texture(texture4, uv);
    if (id == 5)
    return texture(texture5, uv);
    if (id == 6)
    return texture(texture6, uv);
    if (id == 7)
    return texture(texture7, uv);

    return texture(texture0, uv);;
}

layout( push_constant ) uniform constants
{
    vec2 scale;
    vec2 pos;
	uint texture_index;
} PushConstants;

void main() {
	vec4 texel = GetTexelById(PushConstants.texture_index, fragUV);
    outColor = vec4(texel.rgb, 1.0);
}