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
vec3 dayColor = vec3(0.1, 0.3, 0.8);  // Colore del cielo di giorno
vec3 nightColor = vec3(0.0, 0.0, 0.1);  // Colore del cielo di notte

// Funzione per calcolare il colore del cielo in base alla direzione della luce
vec3 calculateSkyColor(vec3 lightDir) {

    vec3 normalizedLightDir = normalize(lightDir);
    // Calcola l'elevazione (dot rispetto all'asse verticale)
    float elevation = dot(normalizedLightDir, vec3(0.0, 1.0, 0.0));

    // Mappa l'elevazione da [-1, 1] a [0, 1]
    float t = clamp(elevation * 0.5 + 0.5, 0.0, 1.0);

    // Determina il colore in base alla posizione del sole
    if (t < 0.4) {
        // Notte piena
        return nightColor;
    } else if (t < 0.8) {
        // Transizione tra notte e giorno
        float mixFactor = smoothstep(0.4, 0.8, t);
        return mix(nightColor, dayColor, mixFactor);
    } else {
        // Giorno pieno
        return dayColor;
    }
}

int calculateValore(int val, vec3 lightDir){
vec3 normalizedLightDir = normalize(lightDir);
    // Calcola l'elevazione (dot rispetto all'asse verticale)
    float elevation = dot(normalizedLightDir, vec3(0.0, 1.0, 0.0));

    // Mappa l'elevazione da [-1, 1] a [0, 1]
    float t = clamp(elevation * 0.5 + 0.5, 0.0, 1.0);

    // Determina il colore in base alla posizione del sole
    if (t < 0.4) {
        val=1;
        return val;
    } else{
        val=0;
        return val;
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
    vec3 LightDir = normalize(gubo.lightDir - fragPos);
    vec3 lightColor = gubo.lightColor.rgb;
    vec3 Ambient = 0.05 * Albedo;
    int val=0;

    vec3 brdf = BRDF(ViewerDir, Norm, LightDir, Albedo, vec3(gubo.metallic), gubo.gamma);

    // Calcola il colore del cielo in base alla posizione del sole
    vec3 skyColor = calculateSkyColor(gubo.lightDir);
    val= calculateValore(val, gubo.lightDir);

    // Mescola il colore della texture con il colore del cielo
    if(val==0){
        vec3 finalColor = mix(Albedo, skyColor, 0.99);
        outColor = vec4(brdf * lightColor + finalColor + Ambient, 1.0);
    }else{
        vec3 finalColor = skyColor;
        outColor = vec4(brdf * lightColor + finalColor + Ambient, 1.0);

    }

}