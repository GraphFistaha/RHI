#version 450

layout(location = 0) out vec4 outColor;

layout(std140, push_constant ) uniform constants
{
	mat4 transform;
	vec4 color;
} PushConstants;

void main() {
    outColor = PushConstants.color;
}