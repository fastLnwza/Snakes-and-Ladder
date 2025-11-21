#version 410 core

in vec3 fragColor;
in vec2 fragTexCoord;

out vec4 outColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main()
{
    if (uUseTexture)
    {
        vec4 texColor = texture(uTexture, fragTexCoord);
        
        // Convert colored background (red) to white, keep black pips
        // Use brightness to detect black pips vs colored background
        float avgColor = (texColor.r + texColor.g + texColor.b) / 3.0;
        float maxChannel = max(max(texColor.r, texColor.g), texColor.b);
        
        // If very dark (black pips), show black
        // Otherwise show white (convert red/colored background to white)
        // Use both avgColor and maxChannel to better detect dark vs bright areas
        if (avgColor < 0.2 && maxChannel < 0.3)
        {
            // Very dark -> black pips
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
        else
        {
            // Everything else (including red background and any colored areas) -> white
            outColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
    }
    else
    {
        outColor = vec4(fragColor, 1.0);
    }
}

