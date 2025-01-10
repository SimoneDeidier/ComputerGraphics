#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D textureSampler;

layout(binding = 2) uniform GlobalUniformBufferObject {
	vec4 lightPositions[5];
	vec4 directLightColor;
	vec4 frontLightColor;
	vec4 rearLightColor;
	vec4 eyePos;
	vec4 gammaAndMetallic;
} gubo;

// LAMBERT duffuse + BLINN specular
vec3 BRDF(vec3 v, vec3 n, vec3 l, vec3 md, vec3 ms, float gamma) {

	vec3 diffuse = md * clamp(dot(n, l), 0.0, 1.0);
	vec3 specular = ms * vec3(pow(clamp(dot(n, normalize(v + l)), 0.0, 1.0), gamma));

	return (diffuse + specular);

}

void main() {

	vec3 norm = normalize(fragNormal);
	vec3 viewerDir = normalize(gubo.eyePos.xyz - fragPos);
	vec3 albedo = texture(textureSampler, fragUV).rgb;

	vec3 res = vec3(0.0);

	for(int i = 0; i < 5; i++) {
		vec3 lightDir = normalize(gubo.lightPositions[i].xyz - fragPos);
		vec3 lightColor = vec3(0.0);
		if(i == 0) {
			lightColor = gubo.directLightColor.rgb;
		}
		else if(i == 1 || i == 2) {
			lightColor = gubo.rearLightColor.rgb * pow((1 / length(gubo.lightPositions[i].xyz - fragPos)), 2.0);
		}
		else {
			lightColor = gubo.frontLightColor.rgb * pow((1 / length(gubo.lightPositions[i].xyz - fragPos)), 2.0);
		}
		vec3 lightBRDF = BRDF(viewerDir, norm, lightDir, albedo, vec3(gubo.gammaAndMetallic.y), gubo.gammaAndMetallic.x);
		res += lightBRDF * lightColor;
	}

	vec3 ambient = 0.05 * albedo;
	res += ambient;

	outColor = vec4(res, 1.0); // main color

}