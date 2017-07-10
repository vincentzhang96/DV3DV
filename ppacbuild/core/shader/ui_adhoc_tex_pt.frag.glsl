#version 430
layout(location = 5) uniform sampler2D tex;

in vec4 oColor;
in vec3 oNormal;
in vec2 oTexCoords;
out vec4 color;

void main()
{
    color = texture(tex, oTexCoords) * oColor;
}
