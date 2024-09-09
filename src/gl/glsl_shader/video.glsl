//vertex shader
#version 330 core 

layout (location = 0) in vec3 aPos;

uniform mat4 mvp;
out vec2 texPos;

void main() {
    gl_Position = vec4(vec2(aPos), 1.0, 1.0) * mvp;
    texPos = gl_Position.xy / 2.0 + 0.5;
}

//fragment shader
#version 330 core

in vec2 texCoord;
uniform sampler2D texture;
out vec4 fragColor;

void main() {
    fragColor = texture(texture, texPos);
}