#version 430
layout(location = 2) in vec4 textColor;
layout(location = 3) uniform mat4 projection
layout(location = 4) uniform sampler2D mtl;

in vec2 textureCoords;
out vec4 color;

void main()
{
    vec4 tex = vec4(1.0, 1.0, 1.0, texture(mtl, textureCoords).r);
    color = vec4(textColor, 1.0) * tex;
}
