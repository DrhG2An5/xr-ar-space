#version 460 core

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform float uOpacity;
uniform bool uSelected;
uniform vec4 uBorderColor;
uniform float uBorderWidth;
uniform bool uHovered;
uniform vec4 uHoverColor;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    texColor.a *= uOpacity;

    // Border highlight for selected screens
    if (uSelected) {
        float borderX = step(vTexCoord.x, uBorderWidth) + step(1.0 - uBorderWidth, vTexCoord.x);
        float borderY = step(vTexCoord.y, uBorderWidth) + step(1.0 - uBorderWidth, vTexCoord.y);
        float border = clamp(borderX + borderY, 0.0, 1.0);
        texColor = mix(texColor, uBorderColor, border);
    } else if (uHovered) {
        float hoverWidth = uBorderWidth * 0.5;
        float borderX = step(vTexCoord.x, hoverWidth) + step(1.0 - hoverWidth, vTexCoord.x);
        float borderY = step(vTexCoord.y, hoverWidth) + step(1.0 - hoverWidth, vTexCoord.y);
        float border = clamp(borderX + borderY, 0.0, 1.0);
        texColor = mix(texColor, uHoverColor, border);
    }

    FragColor = texColor;
}
