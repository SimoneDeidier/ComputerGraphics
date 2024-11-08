#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D textureSampler;

layout(binding = 2) uniform GlobalUniformBufferObject {
	vec3 lightDir;
	vec3 lightPos;
	vec4 lightColor;
	vec3 eyePos;
} gubo;

vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, vec3 Ms, float gamma) {

	vec3 Diffuse = Md * clamp(dot(N, L), 0.0, 1.0);
	vec3 Specular = Ms * vec3(pow(clamp(dot(N, normalize(V + L)), 0.0, 1.0), gamma));

	return (Diffuse+Specular);

}

void main() {

	vec3 Norm = normalize(fragNormal);
	vec3 ViewerDir = normalize(gubo.eyePos - fragPos);
	vec3 Albedo = texture(textureSampler, fragUV).rgb;
	float gamma = 128.0f; //prima 128
	float metallic = 1.0f;
	//vec3 LightDir = normalize(gubo.lightDir);
	vec3 LightDir = normalize(gubo.lightPos);
	vec3 lightColor = gubo.lightColor.rgb;
	vec3 Ambient = 0.1f * Albedo;

	vec3 brdf = BRDF(ViewerDir, Norm, LightDir, Albedo, vec3(metallic), gamma);

	outColor = vec4(brdf * lightColor + Ambient, 1.0f); // main color

}