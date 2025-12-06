#version 410 core

in vec4 fragColor;
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
            // Use texture color directly with alpha channel
            // For RGBA textures, use all channels including alpha for transparency
            // For RGB textures, alpha will be 1.0 (fully opaque)
            // For GL_RED textures (fonts), use red channel as alpha and vertex color for RGB
            // GL_RED texture: r has value, g and b are 0, a may be 1.0 or undefined
            // With texture swizzle, all channels (r, g, b, a) should be the same (red channel value)
            // Check if texture is GL_RED font texture (all channels are similar, r has value)
            bool isLikelyFont = (abs(texColor.r - texColor.g) < 0.01) && 
                                (abs(texColor.g - texColor.b) < 0.01) && 
                                (abs(texColor.b - texColor.a) < 0.01) && 
                                (texColor.r > 0.01);
            
            if (isLikelyFont)
            {
                // GL_RED font texture with swizzle - use vertex color (fragColor) for RGB, red channel as alpha
                // This ensures text uses the color we specify (white) instead of texture color
                // The alpha channel from texture determines transparency
                outColor = vec4(fragColor.rgb, texColor.r);
            }
            else
            {
                // RGB or RGBA texture - use texture color with alpha
                // For RGBA textures, alpha channel will be used for transparency
                // This will properly handle transparency for Panel.png
                // Use texture color directly - OpenGL will handle alpha correctly
                outColor = texColor;
            }
        }
    }
    else
    {
        // Use vertex color with alpha
        outColor = fragColor;
    }
}

