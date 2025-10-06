#version 330 core
out vec4 FragColor;

in vec3 FragColorIn;
in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform bool useTexture;
uniform vec4 prismColor;

void main()
{
    if (useTexture) {
        // If texture is enabled, use the texture color
        FragColor = texture(ourTexture, TexCoord);
    } else if (FragColorIn != vec3(0.0, 0.0, 0.0)) {
        // If we have a vertex color, use it
        FragColor = vec4(FragColorIn, 1.0);
    } else {
        // Otherwise use the uniform color
        FragColor = prismColor;
    }
}
