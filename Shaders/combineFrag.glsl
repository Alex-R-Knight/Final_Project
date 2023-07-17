//#version 330 core
//uniform sampler2D diffuseTex;
//uniform sampler2D diffuseLight;
//uniform sampler2D specularLight;
//
//in Vertex{
//	vec2 texCoord;
//} IN;
//
//out vec4 fragColour;
//
//void main(void) {
//
//	vec2 texSize	= textureSize(diffuseTex, 0).xy;
//	vec2 newTexCoord	= gl_FragCoord.xy / texSize;
//
//	//vec4 uv			= texture(rayMarchUV,	IN.texCoord);
//	//vec4 color		= texture(diffuseTex,	uv.xy);
//
//	//// new reflection method
//	//vec4 reflectColour	= texture(rayMarchUV,	IN.texCoord);
//	//
//	//float reflectivity	= texture(reflectivity,	IN.texCoord).r;
//	//
//	//fragColour			= ( reflectivity <= 0.0 ) ? texture(diffuseTex,	IN.texCoord) : mix( texture(diffuseTex,	IN.texCoord), reflectColour, 0.5 );
//
//	// Default reflection with skybox
//	//fragColour			= mix( texture(diffuseTex,	IN.texCoord), reflectColour, texture(reflectivity,	IN.texCoord).r );
//
//
//
//	//fragColour = texture(diffuseTex,	IN.texCoord);
//
//
//	//fragColour = mix(fragColour, color, uv.b);
//
//
//	//if (uv.b == 1.0f) {
//	//	fragColour = color;
//	//}
//
//	//fragColour = (uv.b != 0.0) ? color : vec4(0.0f, 0.0f, 0.0f, 0.0f);
//
//	//fragColour = color;
//
//	//fragColour = texture(diffuseTex,	IN.texCoord);
//}


///Emergency Backup///

#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D diffuseLight;
uniform sampler2D specularLight;

uniform sampler2D illuminationTex;

uniform sampler2D SSAOTex;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColour;
out vec4 fragColour2;

void main(void) {
	vec3 diffuse = texture(diffuseTex, IN.texCoord).xyz;
	vec3 light = texture(diffuseLight, IN.texCoord).xyz;
	vec3 specular = texture(specularLight, IN.texCoord).xyz;

	vec3 indirect = texture(illuminationTex, IN.texCoord).xyz;

	float SSAOVal = texture(SSAOTex, IN.texCoord).x;
	
	fragColour.xyz = diffuse * 0.15 * SSAOVal; // ambient
	fragColour.xyz += diffuse * light; // lambert
	//fragColour.xyz += indirect * 0.5; // indirect
	fragColour.xyz += specular; // Specular



	// SSAO difference test
	fragColour2.xyz = diffuse * 0.15;
	fragColour2.xyz += diffuse * light; // lambert
	fragColour2.xyz += specular; // Specular

	if (texture(diffuseTex, IN.texCoord).a != 1.0) {
		fragColour = vec4(diffuse.xyz, 1.0);

		fragColour2 = vec4(diffuse.xyz, 1.0);
	}

	//if (texture(diffuseTex, IN.texCoord).a < 0.95) {
	//	fragColour = vec4(diffuse.xyz, 1.0);
	//}

	fragColour.a = 1.0;
	fragColour2.a = 1.0;
}