#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D textureSampler;

layout(binding = 2) uniform GlobalUniformBufferObject {
	vec3 lightDir;
	vec4 lightColor;
	vec3 eyePos;
	float gamma;
	float metallic;
} gubo;

// LAMBERT duffuse + BLINN specular
vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, vec3 Ms, float gamma) {

	vec3 Diffuse = Md * clamp(dot(N, L), 0.0, 1.0);
	vec3 Specular = Ms * vec3(pow(clamp(dot(N, normalize(V + L)), 0.0, 1.0), gamma));

	return (Diffuse + Specular);

}

void main() {

	vec3 Norm = normalize(fragNormal);
	vec3 ViewerDir = normalize(gubo.eyePos - fragPos);
	vec3 Albedo = texture(textureSampler, fragUV).rgb;
	vec3 LightDir = normalize(gubo.lightDir);
	vec3 lightColor = gubo.lightColor.rgb;
	vec3 Ambient = 0.05 * Albedo;

	vec3 brdf = BRDF(ViewerDir, Norm, LightDir, Albedo, vec3(gubo.metallic), gubo.gamma);

	// Ambient light
	/*const vec3 cxp = vec3(0.0, 0.0, 0.5); // Darker blue
	const vec3 cxn = vec3(0.0, 0.0, 0.5); // Darker blue
	const vec3 cyp = vec3(0.5, 0.5, 1.0); // Light blue
	const vec3 cyn = vec3(0.1, 0.1, 0.1); // Dark dark grey
	const vec3 czp = vec3(0.0, 0.0, 0.5); // Darker blue
	const vec3 czn = vec3(0.0, 0.0, 0.5); // Darker blue

	vec3 Ambient = 0.05 * ((Norm.x > 0 ? cxp : cxn) * (Norm.x * Norm.x) + (Norm.y > 0 ? cyp : cyn) * (Norm.y * Norm.y) + (Norm.z > 0 ? czp : czn) * (Norm.z * Norm.z));*/

	outColor = vec4(brdf * lightColor + Ambient, 1.0); // main color

}