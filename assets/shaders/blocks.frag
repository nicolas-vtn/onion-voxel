#version 330 core

in vec2 TexCoord;
in vec3 Tint;
in float Facing;
in float Occlusion;

out vec4 FragColor;

uniform sampler2D u_Atlas;
uniform bool u_RenderCutout; // If the shader should discard pixels based on alpha

uniform vec3 u_LightColor; // The color of the light affecting the block
uniform bool u_UseOcclusion; // If the shader should use occlusion
uniform bool u_UseFaceShading; // If the shader should use face shading

vec3 getNormal(float f) {
    int fi = int(f + 0.2); // Safely round to nearest int (0–5)
    if (fi == 0) return vec3(0, 1, 0);   // Top
    if (fi == 1) return vec3(0, -1, 0);  // Bottom
    if (fi == 2) return vec3(0, 0, -1);  // North
    if (fi == 3) return vec3(0, 0, 1);   // South
    if (fi == 4) return vec3(1, 0, 0);   // East
    if (fi == 5) return vec3(-1, 0, 0);  // West
    return vec3(1, 0, 0);                // Fallback
}

float getLightIntensity(float f) {
	int fi = int(f + 0.2); // Safely round to nearest int (0–5)
	if (fi == 0) return 1.0; // Top
	if (fi == 1) return 0.5; // Bottom
	if (fi == 2) return 0.75; // North
	if (fi == 3) return 0.75; // South
	if (fi == 4) return 0.60; // East
	if (fi == 5) return 0.60; // West
	return 1.0; // Fallback
}

void main()
{

    float occlusionVal = 0.0;

    if(u_UseOcclusion) {
        occlusionVal = Occlusion;
    }

    float faceShading = 1.0;
    if(u_UseFaceShading){
        faceShading = getLightIntensity(Facing);
    }

    vec4 texColor = texture(u_Atlas, TexCoord);
    vec3 finalColor = (1.0 - occlusionVal) * faceShading * u_LightColor * texColor.rgb * Tint;

    if(u_RenderCutout && texColor.a < 0.5) {
        discard;
    } else {
        FragColor = vec4(finalColor , texColor.a);
    }
}
