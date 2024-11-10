#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out mat4 trans;
out vec3 ourPos;

void main()
{
    //gl_Position = view * model * vec4(aPos, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    trans = model;
    ourPos = aPos;
}