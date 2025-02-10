#version 450
#extension GL_ARB_separate_shader_objects : enable

/* --- SKYBOX FRAGMENT SHADER ---
 * This shader is used to render the skybox.
 * It computes the color of the sky based on the position of the sun (elevation).
 * The shader has only one GUBO (specific for the sky box).
 * The GUBO contains the position of the sun.
 * This shader does not compute any BRDF function, so the final color is a mix between the texture and the sky color.
 * Between all the input values from the vertex, only the UV coordinates are used to sample the texture.
 */

// Input from the vertex shader
layout(location = 0) in vec3 fragPos;   // Position of the fragment (not used)
layout(location = 1) in vec2 fragUV;    // UV coordinates of the fragment
layout(location = 2) in vec3 fragNormal;    // Normal of the fragment (not used)

layout(location = 0) out vec4 outColor; // Output color of the fragment

layout(set = 0, binding = 1) uniform sampler2D textureSampler;  // Texture sampler

// Global Uniform Buffer Object
layout(set = 0, binding = 2) uniform GlobalUniformBufferObject {
    vec4 directLightPos;    // Position of the direct light
} gubo;

// Function that computes the color of the sky based on the elevation of the sun
vec3 calculateSkyColor(vec3 lightDir) {

    // Different colors of the sky based on the time of the day
    vec3 dayColor = vec3(0.0, 249.0  / 255.0, 1.0);  // Day color
    vec3 nightColor = vec3(25.0 / 255.0, 25.0 / 255.0, 112.0 / 255.0);  // Night color
    vec3 sunsetColor = vec3(242.0 / 255.0, 72.0 / 255.0, 15.0 / 255.0);  // Sunset color
    vec3 normalizedLightDir = normalize(lightDir);  // Normalize the light direction

    // Computation of the elevation ==> dot product between light direction and the normal vector representing Y axis
    // Get values between -1 and 1 (cosine because the vector are normalized)
    float elevation = dot(normalizedLightDir, vec3(0.0, 1.0, 0.0));

    // Map the elevation from [-1, 1] to [0, 1]
    float t = clamp(elevation * 0.5 + 0.5, 0.0, 1.0);

    // Return the correct color based on t
    if (t < 0.3) {
        return nightColor;  // Night
    }
    else if (t < 0.5) {
        // Gradually change the color from night to sunset
        float mixFactor = smoothstep(0.3, 0.5, t);
        return mix(nightColor, sunsetColor, mixFactor);
    }
    else if (t < 0.65) {
        // Gradually change the color from sunset to day
        float mixFactor = smoothstep(0.5, 0.65, t);
        return mix(sunsetColor, dayColor, mixFactor);
    }
    else {
        return dayColor;    // Day
    }
}

void main() {

    vec3 albedo = texture(textureSampler, fragUV).rgb;  // Albedo of the fragment
    vec3 skyColor = calculateSkyColor(gubo.directLightPos.xyz); // Calculate the sky color based on the sun elevation
    vec3 finalColor = mix(albedo, skyColor, 0.75);  // Mix the albedo with the sky color
    vec3 ambient = 0.05 * finalColor;   // Ambient light (5% of the final color)

    outColor = vec4(finalColor + ambient, 1.0); // Output the resulting color

}