#version 410 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec3 inColor;

out vec3 fragColor;

void main()
{
    fragColor = inColor;
    gl_Position = vec4(inPos, 0.0, 1.0);
}

