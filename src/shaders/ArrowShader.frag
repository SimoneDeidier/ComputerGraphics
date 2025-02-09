#version 450
#extension GL_ARB_separate_shader_objects : enable

/* --- FRAGMENT SHADER USED FOR THE ARROW ----
 * This shader is different from the BaseShader.frag, it is used to render the arrow
 * The arrow is not textured, so the UV mapping is not used and we dont need a sampler2D
 * It only considers the point light (pickup/drop off point) for the BRDF function
 * No other lights are considered (no direct light, no street lights, no taxi lights)
 * There is a strong factor of 0.75 for the albedo of the arrow because the arrow has to be visible
 */

// Data coming from the vertex shader
layout(location = 0) in vec3 fragPos;	// Position
layout(location = 1) in vec2 fragUV;	// UV mapping (not used, the arrow is not textured)
layout(location = 2) in vec3 fragNormal;	// Normal

// Output of the fragment shader
layout(location = 0) out vec4 outColor;	// Color

// Global uniform buffer object
layout(binding = 1) uniform GlobalUniformBufferObject {
	vec4 pickupPointPos;	// Position of the pickup point
	vec4 pickupPointCol;	// Color of the pickup point (POINT LIGHT)
	vec4 eyePos;	// Position of the camera
	vec4 gammaAndMetallic;	// Gamma and metallic values
} gubo;

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
	vec3 albedo = vec3(1.0, 0.0, 0.0);	// Albedo of the fragment ==> RED

	// Direction of the point light (POINT LIGHT)
	vec3 pointLightDir = normalize(gubo.pickupPointPos.xyz - fragPos);
	// Color of the point light
	vec3 pointLightColor = gubo.pickupPointCol.rgb;
	// Calculate the BRDF
	vec3 pointLightBRDF = BRDF(viewerDir, norm, pointLightDir, albedo, vec3(gubo.gammaAndMetallic.y), gubo.gammaAndMetallic.x);

	// Output the color of the fragment: the sum of the BRDF multiplied by the color of the point light and the albedo
	outColor = vec4(pointLightBRDF * pointLightColor + albedo * 0.75, 1.0);

}