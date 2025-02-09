#version 450
#extension GL_ARB_separate_shader_objects : enable

/* --- BASE VERTEX SHADER ---
 * This is the base vertex shader used to transform the vertex data from object space to clip space.
 * This vertex shader also passes the vertex data to the right fragment shader.
 * This shader is used by all the 3D models in the scene.
 */

// Uniform buffer object
layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat;	// Model-View-Projection matrix
	mat4 mMat;	// Model matrix
	mat4 nMat;	// Normal matrix
} ubo;

// Vertex attributes
layout(location = 0) in vec3 inPosition;	// Vertex position
layout(location = 1) in vec2 inUV;	// Vertex UV coordinates
layout(location = 2) in vec3 inNormal;	// Vertex normal

// Fragment shader outputs (passed to the fragment shader)
layout(location = 0) out vec3 outPoistion;	// Vertex position
layout(location = 1) out vec2 outUV;	// Vertex UV coordinates
layout(location = 2) out vec3 outNormal;	// Vertex normal


void main() {
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);	// Transform the vertex position to clip space
	outPoistion = (ubo.mMat * vec4(inPosition, 1.0)).xyz;	// Transform the vertex position to world space
	outUV = inUV;	// Pass the UV coordinates to the fragment shader
	outNormal = (ubo.nMat * vec4(inNormal, 0.0)).xyz;	// Transform the vertex normal to world space
}