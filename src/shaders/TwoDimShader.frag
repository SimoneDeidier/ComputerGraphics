#version 450
#extension GL_ARB_separate_shader_objects : enable

/* --- TWO DIMENSIONAL FRAGMENT SHADER ---
 * This shader is used to render two dimensional textures.
 * It simply gets the color of the fragment from the texture and outputs it.
 */

// Input from the vertex shader
layout(location = 0) in vec2 fragUV;	// UV coordinates of the fragment

layout(binding = 0) uniform sampler2D textureSampler;	// Texture sampler

layout(location = 0) out vec4 outColor;	// Output color of the fragment

void main() {

	// Get the color of the fragment from the texture and output it
	outColor = vec4(texture(textureSampler, fragUV).rgb, 1.0);
	
}