#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 uProjection;
uniform vec2 uSize;     // size in pixels
uniform vec2 uPos;      // position in pixels (top-left origin)
uniform float uZOffset; // z offset for layering

out vec2 TexCoord;

void main()
{
    // Scale quad from unit space to pixel size
    vec2 scaled = aPos * uSize;

    // Translate to pixel position
    vec2 world = scaled + uPos;

    // Convert to clip space using projection
    gl_Position = uProjection * vec4(world, uZOffset, 1.0);

    TexCoord = aUV;
}
