#version 440

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;    

// must have the same name as its corresponding "in" item in the frag shader
smooth out vec3 vertOutColor;
smooth out vec2 texPos;

void main()
{
    vertOutColor = color;
    texPos = pos.xy;
	gl_Position = vec4(pos, 1.0f);
}

