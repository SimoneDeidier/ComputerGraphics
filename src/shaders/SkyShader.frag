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

// Colori per i vari momenti della giornata
vec3 dayColor = vec3(0.3, 0.5, 1.0);  // Colore del cielo di giorno
vec3 sunsetColor = vec3(1.0, 0.3, 0.0);  // Colore del cielo al tramonto
vec3 nightColor = vec3(0.0, 0.0, 0.5);  // Colore del cielo di notte
vec3 dawnColor = vec3(1.0, 0.6, 0.8); // Colore del cielo all'alba

// Funzione per calcolare il colore del cielo in base alla direzione della luce
vec3 calculateSkyColor(vec3 lightDir) {

    float elevation = dot(normalize(lightDir), vec3(0.0, 1.0, 0.0));

    // Mappa l'elevazione su un intervallo [0, 1]
    float t = clamp(elevation * 0.5 + 0.5, 0.0, 1.0);

    // Determina il colore in base alla posizione del sole
    if (t < 0.3) {
        // Transizione tra notte e alba
        float mixFactor = smoothstep(0.0, 0.3, t);
        return mix(nightColor, sunsetColor, mixFactor);
    } else if (t < 0.6) {
        // Transizione tra alba e giorno
        float mixFactor = smoothstep(0.3, 0.6, t);
        return mix(sunsetColor, dayColor, mixFactor);
    } else if (t < 0.9) {
        // Giorno pieno
        return dayColor;
    } else {
        // Transizione tra giorno e notte
        float mixFactor = smoothstep(0.9, 1.0, t);
        return mix(dayColor, nightColor, mixFactor);
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
    vec3 ViewerDir = normalize(gubo.eyePos - fragPos);
    vec3 Albedo = texture(textureSampler, fragUV).rgb;
    vec3 LightDir = normalize(gubo.lightDir);
    vec3 lightColor = gubo.lightColor.rgb;
    vec3 Ambient = 0.05 * Albedo;

    vec3 brdf = BRDF(ViewerDir, Norm, LightDir, Albedo, vec3(gubo.metallic), gubo.gamma);

    // Calcola il colore del cielo in base alla posizione del sole
    vec3 skyColor = calculateSkyColor(gubo.lightDir);

    // Mescola il colore della texture con il colore del cielo
    vec3 finalColor = mix(Albedo, skyColor, 0.8);

    outColor = vec4(brdf * lightColor + finalColor + Ambient, 1.0);
    
}
