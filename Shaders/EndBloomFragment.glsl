#version 330 core
uniform sampler2D diffuseTex;
//uniform sampler2D bloomTex;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColour;

void main(void) {

	//vec3 colourOut = texture(diffuseTex, IN.texCoord).rgb + texture(bloomTex, IN.texCoord).rgb;

	FragColor = texture(diffuseTex, IN.texCoord);
}