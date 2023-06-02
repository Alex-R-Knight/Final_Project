
#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D diffuseTex_2;

in Vertex{
	vec2 texCoord;
	vec4 colour;
} IN;

out vec4 fragColour;
void main(void) {
	//fragColour = texture(diffuseTex, IN.texCoord) * IN.colour;
	//fragColour = (texture(diffuseTex, IN.texCoord) * vec4(IN.colour)) + (0.5 * vec4(IN.colour));

	fragColour = ((1 - vec4(IN.colour)) * texture(diffuseTex_2, IN.texCoord)) + (vec4(IN.colour) * texture(diffuseTex, IN.texCoord));
}