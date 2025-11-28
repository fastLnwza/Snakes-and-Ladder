#version 410 core

in vec3 fragColor;
in vec2 fragTexCoord;

out vec4 outColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;
uniform bool uDiceTextureMode;

void main()
{
    if (uUseTexture)
    {
        vec4 texColor = texture(uTexture, fragTexCoord);

        if (uDiceTextureMode)
        {
            // Convert colored background (red) to white, keep black pips
            float avgColor = (texColor.r + texColor.g + texColor.b) / 3.0;
            float maxChannel = max(max(texColor.r, texColor.g), texColor.b);
            if (avgColor < 0.2 && maxChannel < 0.3)
            {
                outColor = vec4(0.0, 0.0, 0.0, 1.0);
            }
            else
            {
                outColor = vec4(1.0, 1.0, 1.0, 1.0);
            }
        }
        else
        {
            // For font textures (GL_RED format), use red channel as alpha
            float alpha = texColor.r;
            outColor = vec4(fragColor, alpha);
        }
    }
    else
    {
        outColor = vec4(fragColor, 1.0);
    }
}

