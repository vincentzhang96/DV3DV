#version 430
layout(location = 2) uniform sampler2D mtl;

in vec2 textureCoords;

out vec4 color;

void main()
{
    vec4 texColor = texture(mtl, textureCoords);
    if (texColor.a < 1.0 / 255.0)
    {
        discard;
    }
    color = texColor;
}
