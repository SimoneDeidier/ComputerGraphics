#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D textureSampler;

layout(binding = 2) uniform GlobalUniformBufferObject {
	vec4 directLightPos;
	vec4 directLightColor;
	vec4 taxiLightPos[4];
	vec4 frontLightColor;
	vec4 rearLightColor;
	vec4 frontLightDirection;
	vec4 frontLightCosines;
	vec4 streetLightPos[5];
	vec4 streetLightCol;
	vec4 streetLightDirection;
	vec4 streetLightCosines;
	vec4 pickupPointPos;
	vec4 pickupPointCol;
	vec4 pickupPointOn;
	vec4 eyePos;
	vec4 gammaMetallicSettingsNight;
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

	vec3 ambient = 0.05 * albedo;
	res += ambient;

	vec3 directLightDir = normalize(gubo.directLightPos.xyz - fragPos);
	vec3 directLightColor = gubo.directLightColor.rgb;
	vec3 directLightBRDF = BRDF(viewerDir, norm, directLightDir, albedo, vec3(gubo.gammaMetallicSettingsNight.y), gubo.gammaMetallicSettingsNight.x);
	res += directLightBRDF * directLightColor;

	if(gubo.pickupPointOn.x == 1.0) {
		vec3 pickupPointDir = normalize(gubo.pickupPointPos.xyz - fragPos);
		vec3 pickupPointColor = gubo.pickupPointCol.rgb * dot(pow((5 / length(gubo.pickupPointPos.xyz - fragPos)), 2.0), clamp((dot(normalize(gubo.pickupPointPos.xyz - fragPos), gubo.streetLightDirection.xyz) - gubo.streetLightCosines.y) / (gubo.streetLightCosines.x - gubo.streetLightCosines.y), 0.0, 1.0));
		vec3 pickupPointBRDF = BRDF(viewerDir, norm, pickupPointDir, albedo, vec3(gubo.gammaMetallicSettingsNight.y), gubo.gammaMetallicSettingsNight.x);
		res += pickupPointBRDF * pickupPointColor;
	}

	if(gubo.gammaMetallicSettingsNight.z == 0.0) {
		outColor = vec4(res, 1.0);
		return;
	}

	if(gubo.gammaMetallicSettingsNight.w == 1.0) {
		for(int i = 0; i < 4; i++) {
			vec3 taxiLightDir = normalize(gubo.taxiLightPos[i].xyz - fragPos);
			vec3 taxiLightColor = vec3(0.0);
			if(i < 2) {
				taxiLightColor = gubo.rearLightColor.rgb * pow((1 / length(gubo.taxiLightPos[i].xyz - fragPos)), 2.0);
			}
			else {
				taxiLightColor = gubo.frontLightColor.rgb * dot(pow((3 / length(gubo.taxiLightPos[i].xyz - fragPos)), 2.0), clamp((dot(normalize(gubo.taxiLightPos[i].xyz - fragPos), gubo.frontLightDirection.xyz) - gubo.frontLightCosines.y) / (gubo.frontLightCosines.x - gubo.frontLightCosines.y), 0.0, 1.0));
			}
			vec3 taxiLightBRDF = BRDF(viewerDir, norm, taxiLightDir, albedo, vec3(gubo.gammaMetallicSettingsNight.y), gubo.gammaMetallicSettingsNight.x);
			res += taxiLightBRDF * taxiLightColor;
		}

		if(gubo.gammaMetallicSettingsNight.z == 1.0) {
			outColor = vec4(res, 1.0);
			return;
		}

		for(int i = 0; i < 5; i++) {
			vec3 streetLightDir = normalize(gubo.streetLightPos[i].xyz - fragPos);
			vec3 streetLightColor = gubo.streetLightCol.rgb * dot(pow((10 / length(gubo.streetLightPos[i].xyz - fragPos)), 2.0), clamp((dot(normalize(gubo.streetLightPos[i].xyz - fragPos), gubo.streetLightDirection.xyz) - gubo.streetLightCosines.y) / (gubo.streetLightCosines.x - gubo.streetLightCosines.y), 0.0, 1.0));
			vec3 streetLightBRDF = BRDF(viewerDir, norm, streetLightDir, albedo, vec3(gubo.gammaMetallicSettingsNight.y), gubo.gammaMetallicSettingsNight.x);
			res += streetLightBRDF * streetLightColor;
		}

	}

	outColor = vec4(res, 1.0); // main color

}