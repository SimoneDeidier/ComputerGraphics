#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;


void main() {
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
	outPosition = (ubo.mMat * vec4(inPosition, 1.0f)).xyz;
	outUV = inUV;
	outNormal = (ubo.nMat * vec4(inNormal, 0.0)).xyz;
}