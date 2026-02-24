#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 uProjection;

out vec2 TexCoord;

void main()
{
    gl_Position = uProjection * vec4(aPos, 1.0);
    TexCoord = aUV;
}
