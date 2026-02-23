#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uTextColor;   // Text tint color (e.g. white = vec3(1.0))

void main()
{
    vec4 sampled = texture(uTexture, TexCoord);

    // Use alpha from texture
    if (sampled.a < 0.1)
        discard;

    FragColor = vec4(uTextColor, sampled.a);
}
