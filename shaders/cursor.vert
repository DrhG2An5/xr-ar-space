#version 460 core

layout(location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform vec2 uPosition;
uniform float uSize;

void main() {
    vec2 pos = aPos * uSize + uPosition;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
}
