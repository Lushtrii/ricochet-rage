#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;
uniform float light_up;

uniform bool distort_on;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE THE WATER DISTORTION HERE (you may want to try sin/cos)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		if (!distort_on) {
		return uv;  
	}

	float horizontal_wave = 0.05 * sin(time * 0.07 + uv.y * 8.0) + 1.0; 
    float vertical_wave = 0.05 * cos(time * 0.07 + uv.x * 8.0) + 1.0;

	uv = (uv - 0.5f) * vec2(horizontal_wave, vertical_wave) + 0.5f; 

    uv = clamp(uv, 0.0, 1.0);

    return uv;
}

vec4 color_shift(vec4 in_color) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE THE COLOR SHIFTING HERE (you may want to make it blue-ish)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if (!distort_on) {
		return in_color;  
	}

	return in_color += 0.30 * vec4(0.1, 0.0, 0.3, 0); 
}

vec4 fade_color(vec4 in_color) 
{
	if (darken_screen_factor > 0)
		in_color -= darken_screen_factor * vec4(0.8, 0.8, 0.8, 0);

	if (light_up > 0)
        in_color += light_up * vec4(0.3, 0.3, 0.3, 0);

    return clamp(in_color, 0.0, 1.0);
}

void main()
{
	vec2 coord = distort(texcoord);

    vec4 in_color = texture(screen_texture, coord);
    color = color_shift(in_color);
    color = fade_color(color);
}
