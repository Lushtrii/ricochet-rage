#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in float side;

uniform mat4 projection;
uniform float thickness;

void main()
{
    vec2 pos = position;
    vec2 normal;
    
    // Calculate normal vector for the line
    if (gl_VertexID % 2 == 1) {
        vec2 nextPos = position + vec2(1.0, 0.0);
        normal = normalize(vec2(-(nextPos.y - pos.y), nextPos.x - pos.x));
    } else {
        vec2 prevPos = position - vec2(1.0, 0.0);
        normal = normalize(vec2(-(pos.y - prevPos.y), pos.x - prevPos.x));
    }
    
    pos += normal * thickness * side;
    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
