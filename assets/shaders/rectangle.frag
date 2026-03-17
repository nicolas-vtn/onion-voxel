#version 330 core

out vec4 FragColor;

uniform vec4 uColor; // Rectangle color (including alpha)

void main()
{
    FragColor = uColor;
}
