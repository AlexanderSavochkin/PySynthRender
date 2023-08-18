#version 330 core
out vec4 FragColor;

uniform vec4 draw_color;

void main()
{
    FragColor = draw_color;
} 
