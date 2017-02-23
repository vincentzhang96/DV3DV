#version 430
layout(location = 5) uniform sampler2D background;
layout(location = 6) uniform sampler2D vignette;
layout(location = 7) uniform float vignetteStrength;

in vec4 oColor;
in vec3 oNormal;
in vec2 oTexCoords;
out vec4 color;

void main()
{
    vec4 bg = texture(background, oTexCoords);
    vec4 vg = texture(vignette, oTexCoords) * vignetteStrength;
    color = bg * color * vec4(vg.xyz, 1.0);
    color = vec4(1.0, 0.0, 0.0, 1.0);
}
