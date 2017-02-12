#version 430
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 textColor;
layout(location = 2) in vec2 uvCoords;
layout(location = 3) uniform mat4 projection;
layout(location = 4) uniform sampler2D mtl;

out vec2 textureCoords;
out vec4 txtColor;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    textureCoords = vec2(uvCoords.x, 1.0 - uvCoords.y);
    txtColor = textColor;
}
