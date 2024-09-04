//vertex shader
#version 330 core 

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 outColor;
uniform mat4 mvp;

void main() {
    gl_Position = vec4(aPos, 1.0) * mvp;
    outColor = aColor;
}

//fragment shader
#version 330 core

in vec3 outColor;
out vec4 fragColor;

void main() {
    fragColor = vec4(outColor, 1.0);
}