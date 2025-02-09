#version 450
#extension GL_ARB_separate_shader_objects : enable

/* --- BASE FRAGMENT SHADER ----
 * This shader is used by the majority of the objects in the scene (taxi, NPCs, city and people)
 * It considers the direct light, the taxi lights, the street lights and the pickup point light
 * Taxi lights and street lights are only considered if is night (sun below the horizon)
 * As seen in the code, some values are stored in the global uniform buffer object (gubo) and some in the local uniform buffer object (lubo)
 * The shader accounts also for the graphics settings (low, medium, high)
 * In fact if it is setted to low, only the direct light, the pickup point light and the ambient are considered
 * If it is setted to medium, the taxi lights are also considered
 * If it is setted to high, the street lights are also considered (all the light sources)
 */

// Data coming from the vertex shader
layout(location = 0) in vec3 fragPos;	// Position
layout(location = 1) in vec2 fragUV;	// UV mapping
layout(location = 2) in vec3 fragNormal;	// Normal

// Output of the fragment shader
layout(location = 0) out vec4 outColor;	// Color

// Sampler 2D for the texture
layout(set = 0, binding = 1) uniform sampler2D textureSampler;

// Global uniform buffer object
layout(set = 1, binding = 0) uniform GlobalUniformBufferObject {
	vec4 directLightPos;	// Position of the direct light
	vec4 directLightColor;	// Color of the direct light
	vec4 taxiLightPos[4];	// Position of the taxi lights
	vec4 frontLightColor;	// Color of the front taxi light
	vec4 rearLightColor;	// Color of the rear taxi light
	vec4 frontLightDirection;	// Direction of the front taxi light (SPOT LIGHT)
	vec4 frontLightCosines;	// Cosines of the front taxi light
	vec4 streetLightCol;	// Color of the street lights
	vec4 streetLightDirection;	// Direction of the street lights (SPOT LIGHT)
	vec4 streetLightCosines;	// Cosines of the street lights
	vec4 pickupPointPos;	// Position of the pickup point
	vec4 pickupPointCol;	// Color of the pickup point (POINT LIGHT)
	vec4 eyePos;	// Position of the camera
	vec4 settingsAndNight;	// Settings and night values
} gubo;

// Local uniform buffer object
layout(set = 0, binding = 2) uniform LocalUniformBufferObject {
		vec4 streetLightPos[5];	// Position of the street lights
		vec4 gammaAndMetallic;	// Gamma and metallic values
} lubo;

/* BRDF function, used to calculate the color of the fragment, parameters:
 * - v: viewer direction
 * - n: normal of the fragment
 * - l: light direction
 * - md: diffuse material
 * - ms: specular material
 * - gamma: gamma value
 */
vec3 BRDF(vec3 v, vec3 n, vec3 l, vec3 md, vec3 ms, float gamma) {

	vec3 diffuse = md * clamp(dot(n, l), 0.0, 1.0);	// Lambertian diffuse component
	vec3 specular = ms * vec3(pow(clamp(dot(n, normalize(v + l)), 0.0, 1.0), gamma));	// Blinn-Phong specular component

	return (diffuse + specular);	// Return the sum of the two components

}

void main() {

	vec3 norm = normalize(fragNormal);	// Normal of the fragment
	vec3 viewerDir = normalize(gubo.eyePos.xyz - fragPos);	// Viewer direction
	vec3 albedo = texture(textureSampler, fragUV).rgb;	// Albedo of the fragment

	vec3 res = vec3(0.0);	// Initialization of the resulting color

	vec3 ambient = 0.05 * albedo;	// Ambient light (5% of the albedo)
	res += ambient;	// Add the ambient light to the resulting color

	// In all the graphics settings, the direct light is always considered
	vec3 directLightDir = normalize(gubo.directLightPos.xyz - fragPos);	// Direction of the direct light
	vec3 directLightColor = gubo.directLightColor.rgb;	// Color of the direct light
	// Calculate the BRDF of the fragment with the direct light
	vec3 directLightBRDF = BRDF(viewerDir, norm, directLightDir, albedo, vec3(lubo.gammaAndMetallic.y), lubo.gammaAndMetallic.x);
	// Add the direct light to the resulting color
	res += directLightBRDF * directLightColor;

	// Also the pickup (or drop off) point light is always considered in all the graphics settings
	vec3 pickupPointDir = normalize(gubo.pickupPointPos.xyz - fragPos);	// Direction of the pickup point light
	// Color of the pickup point light
	vec3 pickupPointColor = gubo.pickupPointCol.rgb * pow((5 / length(gubo.pickupPointPos.xyz - fragPos)), 2.0);
	// Calculate the BRDF of the fragment with the pickup point light
	vec3 pickupPointBRDF = BRDF(viewerDir, norm, pickupPointDir, albedo, vec3(lubo.gammaAndMetallic.y), lubo.gammaAndMetallic.x);
	res += pickupPointBRDF * pickupPointColor;	// Add the pickup point light to the resulting color

	// If the graphics settings are setted to low:
	if(gubo.settingsAndNight.x == 0.0) {
		outColor = vec4(res, 1.0);	// Output the resulting color (direct light + pickup point light + ambient light)
		return;	// Exit the shader calculation
	}

	// If the night flag is setted to 1 (true):
	if(gubo.settingsAndNight.y == 1.0) {
		// For all the taxi lights:
		for(int i = 0; i < 4; i++) {
			vec3 taxiLightDir = normalize(gubo.taxiLightPos[i].xyz - fragPos);	// Direction of the taxi light
			vec3 taxiLightColor = vec3(0.0);	// Color of the taxi light initially setted to 0
			// If the taxi light is a rear light:
			if(i < 2) {
				// The color for the rear lght is considered as red POINT LIGHT
				taxiLightColor = gubo.rearLightColor.rgb * pow((1 / length(gubo.taxiLightPos[i].xyz - fragPos)), 2.0);
			}
			else {
				// Else the color for the front light is considered as yellow SPOT LIGHT 
				taxiLightColor = gubo.frontLightColor.rgb * dot(pow((3 / length(gubo.taxiLightPos[i].xyz - fragPos)), 2.0), clamp((dot(normalize(gubo.taxiLightPos[i].xyz - fragPos), gubo.frontLightDirection.xyz) - gubo.frontLightCosines.y) / (gubo.frontLightCosines.x - gubo.frontLightCosines.y), 0.0, 1.0));
			}
			// Calculate the BRDF of the fragment for each taxi light
			vec3 taxiLightBRDF = BRDF(viewerDir, norm, taxiLightDir, albedo, vec3(lubo.gammaAndMetallic.y), lubo.gammaAndMetallic.x);
			res += taxiLightBRDF * taxiLightColor;	// Add the calculated taxi light to the resulting color
		}

		// If the graphics settings are setted to medium:
		if(gubo.settingsAndNight.x == 1.0) {
			outColor = vec4(res, 1.0);	// Output the resulting color (direct light + pickup point light + ambient light + taxi lights)
			return;	// Exit the shader calculation
		}

		// For each street light:
		for(int i = 0; i < 5; i++) {
			vec3 streetLightDir = normalize(lubo.streetLightPos[i].xyz - fragPos);	// Direction of the street light
			// Color of the street light (yellow SPOT LIGHT)
			vec3 streetLightColor = gubo.streetLightCol.rgb * dot(pow((10 / length(lubo.streetLightPos[i].xyz - fragPos)), 2.0), clamp((dot(normalize(lubo.streetLightPos[i].xyz - fragPos), gubo.streetLightDirection.xyz) - gubo.streetLightCosines.y) / (gubo.streetLightCosines.x - gubo.streetLightCosines.y), 0.0, 1.0));
			// Calculate the BRDF of the fragment for each street light
			vec3 streetLightBRDF = BRDF(viewerDir, norm, streetLightDir, albedo, vec3(lubo.gammaAndMetallic.y), lubo.gammaAndMetallic.x);
			res += streetLightBRDF * streetLightColor;	// Add the calculated street light to the resulting color
		}

	}

	outColor = vec4(res, 1.0); // Output the resulting color (direct light + pickup point light + ambient light + taxi lights + street lights)

}