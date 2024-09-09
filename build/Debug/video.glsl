//vertex shader
#version 330 core 

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 texCoords;

uniform mat4 mvp;
out vec2 texPos;

void main() {
    gl_Position = vec4(vec2(aPos), 0.5, 1.0) * mvp;
    texPos = vec2(texCoords.x, 1.0 - texCoords.y);  // ·­×ª Y ×ø±ê
}

//fragment shader
#version 330 core

in vec2 texPos;
uniform sampler2D texture;
out vec4 fragColor;

void main() {
    fragColor = texture(texture, texPos);
}

