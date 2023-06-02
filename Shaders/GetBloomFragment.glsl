#version 330 core
uniform sampler2D diffuseTex;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColour;
void main(void) {
	fragColour = texture(diffuseTex, IN.texCoord);
	float brightness = dot(texture(diffuseTex, IN.texCoord).rgb, vec3(0.2126, 0.7152, 0.0722));
	if (brightness <= 1.0) {
		fragColour = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
