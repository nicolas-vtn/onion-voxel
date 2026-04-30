#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTextColor;   // Text tint color (e.g. white = vec4(1.0))

void main()
{
    vec4 sampled = texture(uTexture, TexCoord);

    // Use alpha from texture multiplied by color alpha
    float alpha = sampled.a * uTextColor.a;
    if (alpha < 0.1)
        discard;

    FragColor = vec4(uTextColor.rgb, alpha);
}
