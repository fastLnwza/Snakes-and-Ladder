#version 410 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

out vec3 fragColor;

uniform mat4 uMVP;

void main()
{
    fragColor = inColor;
    gl_Position = uMVP * vec4(inPos, 1.0);
}

