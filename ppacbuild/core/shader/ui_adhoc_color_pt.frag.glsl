#version 430

in vec4 oColor;
in vec3 oNormal;
in vec2 oTexCoords;
out vec4 color;

void main()
{
    color = oColor;
}
