#version 430
layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec4 iColor;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec2 iTexCoords;
layout(location = 4) uniform mat4 uProjection;

out vec4 oColor;
out vec3 oNormal;
out vec2 oTexCoords;

void main()
{
    gl_Position = uProjection * vec4(iPosition, 1.0);
    oColor = iColor;
    oNormal = iNormal;
    oTexCoords = iTexCoords;
}
