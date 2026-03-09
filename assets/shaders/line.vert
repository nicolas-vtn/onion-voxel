#version 330 core
layout(location = 0) in vec3 aPos;

uniform bool u_NormalizedCoordonates;
uniform mat4 u_ViewProj;
uniform bool u_TopMost;

void main()
{
    if (u_NormalizedCoordonates)
    {
        // convert [0..1] screen space to NDC [-1..1]
        vec2 ndc = aPos.xy * 2.0 - 1.0;
        gl_Position = vec4(ndc, 0.0, 1.0);
    }
    else
    {
        gl_Position = u_ViewProj * vec4(aPos, 1.0);
    }

    if (u_TopMost)
        gl_Position.z = -1.0;
}
