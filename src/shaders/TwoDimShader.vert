#version 450
#extension GL_ARB_separate_shader_objects : enable

/* --- TWO DIMENSIONAL VERTEX SHADER ---
 * This shader is used to render 2D objects.
 * It takes in the vertex position and UV coordinates as input and passes the UV coordinates to the fragment shader.
 */

// Input vertex data
layout(location = 0) in vec3 inPosition;	// Vertex position
layout(location = 1) in vec2 inUV;	// Vertex UV

// Output data to fragment shader
layout(location = 0) out vec2 outUV;	// UV coordinate


void main() {

	gl_Position = vec4(inPosition, 1.0);	// Set vertex position
	outUV = inUV;	// Set UV coordinate

}