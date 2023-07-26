
//Original code

#version 330 core
uniform sampler2D diffuseTex;

uniform sampler2D rayMarchUV;

uniform sampler2D reflectivity;

uniform sampler2D blurTex;

uniform sampler2D sobelTex;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColour;
void main(void) {
	vec4 reflectColour	= texture(rayMarchUV,	IN.texCoord);
	
	float reflectivity	= texture(reflectivity,	IN.texCoord).r;

	vec4 baseColour		= texture(diffuseTex,	IN.texCoord);

	vec4 blurColour		= texture(blurTex,		IN.texCoord);

	//float sobelVal		= texture(sobelTex,		IN.texCoord).r;
	vec2 sobelVal		= texture(sobelTex,		IN.texCoord).xy;
	
	//fragColour			= ( reflectivity <= 0.0 || sobelVal == 1.0 ) ? mix(baseColour, blurColour, sobelVal) : mix( texture(diffuseTex, IN.texCoord), reflectColour, 0.5 );

	vec4 endReflectiveColour = mix( texture(diffuseTex, IN.texCoord), reflectColour, 0.5 );

	vec4 unreflectiveColour = (sobelVal.x == 1.0) ? mix(blurColour, baseColour, 0.5*sobelVal.y) : baseColour;

	fragColour			= ( reflectivity <= 0.0 ) ? unreflectiveColour : endReflectiveColour;


	//fragColour			= ( reflectivity <= 0.0 ) ? texture(diffuseTex,	IN.texCoord) : mix( texture(diffuseTex,	IN.texCoord), reflectColour, 0.5 );

	//fragColour = texture(diffuseTex, IN.texCoord);
}
