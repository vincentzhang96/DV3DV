#version 430
layout(location = 2) uniform vec4 textColor;
layout(location = 4) uniform sampler2D mtl;

in vec2 textureCoords;
out vec4 color;

void main()
{
    vec4 tex = vec4(1.0, 1.0, 1.0, texture(mtl, textureCoords).r);
    color = textColor * tex;
}
