#version 330 core
layout (location = 0) in ivec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in uint aFacing;
layout (location = 3) in uint aOcclusion;
layout (location = 4) in vec3 aTint;

out vec2 TexCoord;
out vec3 Tint;
flat out uint Facing;
out float Occlusion;

uniform mat4 u_ViewProjMatrix;

uniform ivec2 u_ChunkOffset;
uniform vec3 u_CameraPosition;

void main()
{
    float occlusionMin = 0.1;
    float occlusionMax = 0.7;
    float fOcclusion = float(aOcclusion) / 255.0;
    float remappedOclusion = occlusionMin + (occlusionMax - occlusionMin) * fOcclusion;

    vec3 offsetChunk = vec3(u_ChunkOffset.x, 0, u_ChunkOffset.y);
    vec3 totalOffset = offsetChunk - u_CameraPosition;
    vec3 pos = vec3(aPos.x / 32.0, aPos.y / 32.0, aPos.z / 32.0) + totalOffset;
    gl_Position = u_ViewProjMatrix * vec4(pos, 1.0);

     // --- World position ---
    vec4 worldPos = vec4(pos, 1.0);

    TexCoord = aTexCoord;
    Tint = aTint;
    Facing = aFacing;
    Occlusion = remappedOclusion;
}
