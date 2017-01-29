#version 330 core

out vec4 color;
uniform vec3 pointColor;

void main()
{    
    color = vec4(pointColor, 0.0);
} 