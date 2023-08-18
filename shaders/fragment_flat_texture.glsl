#version 330 core

in vec3 fragment_position;
in vec2 fragment_texture_coords;

out vec3 color;

uniform sampler2D texture1;

void main()
{
    color = texture(texture1, fragment_texture_coords).rgb;
}

