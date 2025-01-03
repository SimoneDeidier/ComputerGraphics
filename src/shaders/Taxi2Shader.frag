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
	vec3 rearLightPos;
	vec4 rearLightColor;
	vec3 eyePos;
	float gamma;
	float metallic;
} gubo;

// LAMBERT duffuse + BLINN specular
vec3 BRDF(vec3 v, vec3 n, vec3 l, vec3 md, vec3 ms, float gamma) {

	vec3 diffuse = md * clamp(dot(n, l), 0.0, 1.0);
	vec3 specular = ms * vec3(pow(clamp(dot(n, normalize(v + l)), 0.0, 1.0), gamma));

	return (diffuse + specular);

}

void main() {

	vec3 norm = normalize(fragNormal);
	vec3 viewerDir = normalize(gubo.eyePos - fragPos);
	vec3 albedo = texture(textureSampler, fragUV).rgb;
	vec3 directLightDir = normalize(gubo.lightDir);
	vec3 directLightColor = gubo.lightColor.rgb;
	vec3 ambient = 0.05 * albedo;

	vec3 directLightBRDF = BRDF(viewerDir, norm, directLightDir, albedo, vec3(gubo.metallic), gubo.gamma);

	vec3 rearlightDir = normalize(gubo.rearLightPos - fragPos);
	vec3 rearLightColor = gubo.rearLightColor.rgb * pow((1.0 / length(gubo.rearLightPos - fragPos)), 2.0);

	vec3 rearLightBRDF = BRDF(viewerDir, norm, rearlightDir, albedo, vec3(gubo.metallic), gubo.gamma);

	outColor = vec4(directLightBRDF * directLightColor + rearLightBRDF * rearLightColor  + ambient, 1.0); // main color

}