#version 430
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uvCoords;

out vec2 textureCoords;

void main()
{
    gl_Position = vec4(position, 1.0);
    textureCoords = vec2(uvCoords.x, uvCoords.y);
}
