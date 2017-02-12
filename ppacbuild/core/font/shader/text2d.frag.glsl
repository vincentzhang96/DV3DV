#version 430
layout(location = 4) uniform sampler2D mtl;

in vec2 textureCoords;
in vec4 txtColor;
out vec4 color;

void main()
{
    vec4 tex = vec4(1.0, 1.0, 1.0, texture(mtl, textureCoords).r);
    color = txtColor * tex;
}
