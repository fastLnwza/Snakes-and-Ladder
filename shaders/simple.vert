#version 410 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inTexCoord;

out vec4 fragColor;
out vec2 fragTexCoord;

uniform mat4 uMVP;

void main()
{
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    gl_Position = uMVP * vec4(inPos, 1.0);
}

