#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec4 prismColor;
uniform sampler2D ourTexture;
uniform bool useTexture;

void main()
{
    if (useTexture) {
        FragColor = texture(ourTexture, TexCoord);
    } else {
        FragColor = prismColor;
    }
}
