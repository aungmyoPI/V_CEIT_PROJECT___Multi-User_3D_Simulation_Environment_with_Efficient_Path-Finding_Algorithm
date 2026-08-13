#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 lightDir;
uniform vec3 viewPos;

const vec2 sampleDisk[8] = vec2[](
    vec2(-0.326212, -0.405810),
    vec2(-0.840144, -0.073580),
    vec2(-0.695914,  0.457137),
    vec2(-0.203345,  0.620716),
    vec2( 0.962340, -0.194983),
    vec2( 0.473434, -0.480026),
    vec2( 0.519456,  0.767022),
    vec2( 0.185461, -0.893124)
);

float pseudoRandom(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal  = texture(gNormal, TexCoords).rgb;
    vec3 Albedo  = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

    // Default sky/background color
    vec3 fogColor = vec3(0.08, 0.10, 0.14); // Matches a soft dark/misty atmosphere

    // If sampling empty background (sky), render sky fog color directly
    if (length(Normal) < 0.1) {
        FragColor = vec4(fogColor, 1.0);
        return;
    }

    // --- 1. SCREEN-SPACE OCCLUSION (AO) ---
    float occlusion = 0.0;
    vec2 texelSize = 1.0 / textureSize(gPosition, 0);
    float viewDist = length(viewPos - FragPos);
    float radiusPixels = clamp(20.0 / max(viewDist * 0.15, 1.0), 3.0, 12.0);
    float maxDistance = 0.6;

    for (int i = 0; i < 8; ++i) {
        vec2 offset = sampleDisk[i] * radiusPixels * texelSize;
        vec2 sampleUV = clamp(TexCoords + offset, vec2(0.0), vec2(1.0));

        vec3 samplePos  = texture(gPosition, sampleUV).rgb;
        vec3 sampleNorm = texture(gNormal, sampleUV).rgb;

        vec3 dir = samplePos - FragPos;
        float dist = length(dir);

        if (dist > 0.01 && dist < maxDistance && length(sampleNorm) > 0.1) {
            vec3 normDir = dir / dist;
            float NdotD = max(0.0, dot(Normal, normDir));
            float normSimilarity = max(0.0, dot(Normal, sampleNorm));
            float falloff = 1.0 - (dist / maxDistance);

            occlusion += NdotD * falloff * (1.0 - normSimilarity);
        }
    }

    float aoFactor = clamp(1.0 - (occlusion / 8.0) * 1.5, 0.35, 1.0);

    // --- 2. DIRECT LIGHTING PASS ---
    vec3 lightColor = vec3(1.0, 0.95, 0.8);
    vec3 ambient = 0.25 * Albedo * lightColor * aoFactor;

    vec3 norm = normalize(Normal);
    vec3 invLightDir = normalize(-lightDir);
    float diff = max(dot(norm, invLightDir), 0.0);
    vec3 diffuse = diff * Albedo * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(invLightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular = lightColor * spec * Specular;

    vec3 finalLitColor = ambient + diffuse + specular;

    // --- 3. FOG CALCULATION ---
    // Distance from camera to the surface fragment
    float distanceToCamera = length(viewPos - FragPos);

    // Controls how thick the fog gets over distance
    float fogDensity = 0.018;

    // Exponential squared fog factor: 0.0 = fully fogged, 1.0 = fully clear
    float fogFactor = exp(-pow(distanceToCamera * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Blend the scene lighting with the fog color based on distance
    vec3 finalColorWithFog = mix(fogColor, finalLitColor, fogFactor);

    FragColor = vec4(finalColorWithFog, 1.0);
}
