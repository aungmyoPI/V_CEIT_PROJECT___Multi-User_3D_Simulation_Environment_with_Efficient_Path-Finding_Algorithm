#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 7) in mat4 aInstanceMatrix;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPosition = aInstanceMatrix * vec4(aPos, 1.0);
    FragPos = vec3(worldPosition);
    TexCoords = aTexCoords;

    // Normal transformation
    mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));
    Normal = normalMatrix * aNormal;

    gl_Position = projection * view * worldPosition;
}
