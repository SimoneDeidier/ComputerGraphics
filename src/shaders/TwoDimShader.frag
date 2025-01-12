#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragUV;

layout(binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main() {

	outColor = vec4(texture(textureSampler, fragUV).rgb, 1.0);
	
}