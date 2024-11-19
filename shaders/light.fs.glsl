#version 330

// From vertex shader
// in vec2 texcoord;
in float shadow_on_out;

// Application data
// uniform sampler2D sampler0;
// uniform vec3 fcolor;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	// color = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));
    if (shadow_on_out > 0.5f) {
    color = vec4(0.0, 0.0, 0.0, 0.2);
    }
    else {
    color = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
