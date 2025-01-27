#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D textureSampler;

layout(binding = 2) uniform GlobalUniformBufferObject {
    vec4 lightDir;
    vec4 lightColor;
    vec4 eyePos;
    vec4 gammaAndMetallic;
} gubo;

// Colors for the moments of the day
vec3 dayColor = vec3(0.0, 249.0  / 255.0, 1.0);  //day color
vec3 nightColor = vec3(25.0 / 255.0, 25.0 / 255.0, 112.0 / 255.0);  //night color
vec3 sunsetColor = vec3(242.0 / 255.0, 72.0 / 255.0, 15.0 / 255.0);  //sunset color

//function that compute the color of the sky based on the elevation of the sun
vec3 calculateSkyColor(vec3 lightDir) {

    vec3 normalizedLightDir = normalize(lightDir);
    // Computation of the elevation -> dot product between light dir and the normal vector representing y axis
    // Get values between -1 and 1 (I get the cosine because the vector are normalized)
    float elevation = dot(normalizedLightDir, vec3(0.0, 1.0, 0.0));

    // Map the elevation from [-1, 1] to [0, 1]
    float t = clamp(elevation * 0.5 + 0.5, 0.0, 1.0);

    // Return the correct color based on t
    if (t < 0.3) {
        // Night
        return nightColor;
    } else if (t < 0.5) {
        //Gradually change the color from night to sunset
        float mixFactor = smoothstep(0.3, 0.5, t);
        return mix(nightColor, sunsetColor, mixFactor);
    }else if (t < 0.65) {
        //Gradually change the color from sunset to day
        float mixFactor = smoothstep(0.5, 0.65, t);
        return mix(sunsetColor, dayColor, mixFactor);
    } else {
        // Day
        return dayColor;
    }
}

// Funzione BRDF (Lambertian + Blinn-Phong)
vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, vec3 Ms, float gamma) {
    vec3 Diffuse = Md * clamp(dot(N, L), 0.0, 1.0);
    vec3 Specular = Ms * vec3(pow(clamp(dot(N, normalize(V + L)), 0.0, 1.0), gamma));
    return (Diffuse + Specular);
}

void main() {
    vec3 Norm = normalize(fragNormal);
    vec3 ViewerDir = normalize(gubo.eyePos.xyz - fragPos);
    vec3 Albedo = texture(textureSampler, fragUV).rgb;
    vec3 LightDir = normalize(gubo.lightDir.xyz - fragPos);
    vec3 lightColor = gubo.lightColor.rgb;
    vec3 Ambient = 0.05 * Albedo;
    int val=0;

    vec3 brdf = BRDF(ViewerDir, Norm, LightDir, Albedo, vec3(gubo.gammaAndMetallic.y), gubo.gammaAndMetallic.x);

    // Calcola il colore del cielo in base alla posizione del sole
    vec3 skyColor = calculateSkyColor(gubo.lightDir.xyz);

    vec3 finalColor = mix(Albedo, skyColor, 0.75);

    outColor = vec4(brdf * lightColor+ finalColor + Ambient, 1.0);

}