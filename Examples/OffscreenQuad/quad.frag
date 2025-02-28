#version 450

layout(location = 0) in vec2 uv;
layout(binding = 0) uniform sampler2D quad_texture;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(quad_texture, uv);
}