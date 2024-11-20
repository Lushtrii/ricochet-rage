#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;

uniform bool distort_on;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) 
{
	return uv;
}

vec4 color_shift(vec4 in_color) 
{
	if (!distort_on) {
		return in_color;  
	}

	float wave = 0.01 * sin(time * 0.25) + 1.0;
	in_color.x = clamp(in_color.x * wave, 0.0, 1.0);
	return in_color;
}

vec4 fade_color(vec4 in_color) 
{
	if (darken_screen_factor > 0)
		in_color -= darken_screen_factor * vec4(0.8, 0.8, 0.8, 0);
	return in_color;
}

void main()
{
	vec2 coord = distort(texcoord);

    vec4 in_color = texture(screen_texture, coord);
    color = color_shift(in_color);
    color = fade_color(color);
}
