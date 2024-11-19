#version 330

// !!! Simple shader for colouring basic meshes

// Input attributes
in vec3 in_position;

// Application data
// uniform mat3 transform;
uniform mat3 projection;
uniform float shadow_on;

out float shadow_on_out;

void main()
{
	vec3 pos = projection * vec3(in_position.xy, 1.0);
    shadow_on_out = shadow_on;
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
