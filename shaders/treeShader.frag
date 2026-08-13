#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);

    // Discard pixels where the leaf texture is transparent
    if (texColor.a < 0.25) {
        discard;
    }

    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = texColor.rgb;
    gAlbedoSpec.a = 0.1; // Low specular for natural leaves
}
