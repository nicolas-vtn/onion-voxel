#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in uint aFacing;
layout (location = 3) in vec3 aTint;

out vec2 TexCoord;
out vec3 Tint;
flat out uint Facing;

uniform mat4 u_ViewProjMatrix;
uniform vec2 u_PositionOffset;

void main()
{
    vec3 pos = aPos + vec3(u_PositionOffset, 0.0);
    gl_Position = u_ViewProjMatrix * vec4(pos, 1.0);

    TexCoord = aTexCoord;
    Tint = aTint;
    Facing = aFacing;
}
