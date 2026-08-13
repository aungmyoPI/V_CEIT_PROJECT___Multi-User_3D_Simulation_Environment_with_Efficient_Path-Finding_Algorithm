#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 7) in mat4 aInstanceMatrix;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 projection;
uniform mat4 view;
uniform float time;

void main()
{
    TexCoords = aTexCoords;
    vec4 worldPos = aInstanceMatrix * vec4(aPos, 1.0f);

    // Wind Simulation
    if (aPos.y > 0.2f)
    {
        float waveSpeed = 2.5f;
        float waveStrength = 0.12f;
        float phaseOffset = worldPos.x * 0.3f + worldPos.z * 0.3f;
        worldPos.x += sin(time * waveSpeed + phaseOffset) * waveStrength;
    }

    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(aInstanceMatrix))) * aNormal;
    gl_Position = projection * view * worldPos;
}
