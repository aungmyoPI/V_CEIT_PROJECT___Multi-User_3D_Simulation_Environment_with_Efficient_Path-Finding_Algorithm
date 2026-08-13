#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    if(texColor.a < 0.1)
        discard;

    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = texColor.rgb;
    gAlbedoSpec.a = 0.1; // Moderate specular strength
}
