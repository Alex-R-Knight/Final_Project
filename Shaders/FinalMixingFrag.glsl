
//Original code

#version 330 core
uniform sampler2D diffuseTex;

uniform sampler2D rayMarchUV;

uniform sampler2D reflectivity;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColour;
void main(void) {
	vec4 reflectColour	= texture(rayMarchUV,	IN.texCoord);
	
	float reflectivity	= texture(reflectivity,	IN.texCoord).r;
	
	fragColour			= ( reflectivity <= 0.0 ) ? texture(diffuseTex,	IN.texCoord) : mix( texture(diffuseTex,	IN.texCoord), reflectColour, 0.5 );

	//fragColour = texture(diffuseTex, IN.texCoord);
}
