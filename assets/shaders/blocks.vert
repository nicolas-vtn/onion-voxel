#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in float aFacing;
layout (location = 3) in float aOcclusion;
layout (location = 4) in vec3 aTint;

out vec2 TexCoord;
out vec3 Tint;
out float Facing;
out float Occlusion;

uniform mat4 u_ViewProjMatrix;

void main()
{
    float occlusionMin = 0.1;
    float occlusionMax = 0.7;
    float remappedOclusion = occlusionMin + (occlusionMax - occlusionMin) * aOcclusion;

    gl_Position = u_ViewProjMatrix * vec4(aPos, 1.0);

     // --- World position ---
    vec4 worldPos = vec4(aPos, 1.0);

    TexCoord = aTexCoord;
    Tint = aTint;
    Facing = aFacing;
    Occlusion = remappedOclusion;
}
