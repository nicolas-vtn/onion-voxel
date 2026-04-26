#version 330 core

in vec2 TexCoord;
in vec3 Tint;
flat in uint Facing;

out vec4 FragColor;

uniform sampler2D u_Atlas;
uniform bool u_RenderCutout; // If the shader should discard pixels based on alpha

uniform vec3 u_LightColor; // The color of the light affecting the block
uniform bool u_UseFaceShading; // If the shader should use face shading

vec3 getNormal(uint f)
{
    const vec3 table[6] = vec3[6](
        vec3(0,1,0), // Top
        vec3(0,-1,0), // Bottom
        vec3(0,0,-1), // North
        vec3(0,0,1), // South
        vec3(1,0,0), // East
        vec3(-1,0,0) // West
    );

    return table[f];
}

float getLightIntensity(uint f)
{
// Precomputed light intensity values for each face direction (top, bottom, north, south, east, west)
    const float table[6] = float[6](1.0, 0.5, 0.75, 0.75, 0.60, 0.60);
    return table[f];
}

void main()
{
    float faceShading = 1.0;
    if(u_UseFaceShading){
        faceShading = getLightIntensity(Facing);
    }

    vec4 texColor = texture(u_Atlas, TexCoord);
    vec3 finalColor = faceShading * u_LightColor * texColor.rgb * Tint;

    if(u_RenderCutout && texColor.a < 0.5) {
        discard;
    } else {
        FragColor = vec4(finalColor , texColor.a);
    }
}
