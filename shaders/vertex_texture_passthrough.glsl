#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texture_coords;

out vec3 fragment_position;
out vec2 fragment_texture_coords;

void main()
{
    fragment_position = position;
    fragment_texture_coords = texture_coords;
    gl_Position = vec4(position.x, position.y, position.z, 1.0);
}

