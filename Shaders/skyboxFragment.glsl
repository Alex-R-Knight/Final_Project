#version 330 core

uniform samplerCube cubeTex;
in Vertex{
	vec3 viewDir;
} IN;

out vec4 fragColour;
out vec4 clearColour[3];

void main(void) {
	fragColour = vec4(texture(cubeTex, normalize(IN.viewDir)).rgb, 0.9);

	clearColour[0] = vec4(0.0f);
	clearColour[1] = vec4(0.0f);
	clearColour[2] = vec4(0.0f);
}