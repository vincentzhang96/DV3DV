#version 430
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uvCoords;
layout(location = 3) uniform mat4 projection;

out vec2 textureCoords;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    textureCoords = vec2(uvCoords.x, 1.0 - uvCoords.y);
}
