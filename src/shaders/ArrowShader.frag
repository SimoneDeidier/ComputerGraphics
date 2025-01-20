#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform GlobalUniformBufferObject {
	vec4 pickupPointPos;
	vec4 pickupPointCol;
	vec4 eyePos;
	vec4 gammaAndMetallic;
} gubo;

// Lambert + Blinn
vec3 BRDF(vec3 v, vec3 n, vec3 l, vec3 md, vec3 ms, float gamma) {

	vec3 diffuse = md * clamp(dot(n, l), 0.0, 1.0);
	vec3 specular = ms * vec3(pow(clamp(dot(n, normalize(v + l)), 0.0, 1.0), gamma));

	return (diffuse + specular);

}

void main() {

	vec3 norm = normalize(fragNormal);
	vec3 viewerDir = normalize(gubo.eyePos.xyz - fragPos);
	vec3 albedo = vec3(1.0, 0.0, 0.0);

	vec3 pointLightDir = normalize(gubo.pickupPointPos.xyz - fragPos);
	vec3 pointLightColor = gubo.pickupPointCol.rgb;
	vec3 pointLightBRDF = BRDF(viewerDir, norm, pointLightDir, albedo, vec3(gubo.gammaAndMetallic.y), gubo.gammaAndMetallic.x);

	outColor = vec4(pointLightBRDF * pointLightColor + albedo * 0.75, 1.0);

}