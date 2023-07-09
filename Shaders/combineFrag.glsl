#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D diffuseLight;
uniform sampler2D specularLight;
uniform sampler2D rayMarchUV;

uniform sampler2D reflectivity;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColour;

void main(void) {

	vec2 texSize	= textureSize(diffuseTex, 0).xy;
	vec2 newTexCoord	= gl_FragCoord.xy / texSize;

	//vec4 uv			= texture(rayMarchUV,	IN.texCoord);
	//vec4 color		= texture(diffuseTex,	uv.xy);

	// new reflection method
	vec4 reflectColour	= texture(rayMarchUV,	IN.texCoord);

	fragColour			= mix( texture(diffuseTex,	IN.texCoord), reflectColour, texture(reflectivity,	IN.texCoord).r );

	//fragColour = texture(diffuseTex,	IN.texCoord);


	//fragColour = mix(fragColour, color, uv.b);


	//if (uv.b == 1.0f) {
	//	fragColour = color;
	//}

	//fragColour = (uv.b != 0.0) ? color : vec4(0.0f, 0.0f, 0.0f, 0.0f);

	//fragColour = color;

	//fragColour = texture(diffuseTex,	IN.texCoord);
}


///Emergency Backup///

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
//	vec3 diffuse = texture(diffuseTex, IN.texCoord).xyz;
//	vec3 light = texture(diffuseLight, IN.texCoord).xyz;
//	vec3 specular = texture(specularLight, IN.texCoord).xyz;
//	
//	fragColour.xyz = diffuse * 0.15; // ambient
//	fragColour.xyz += diffuse * light; // lambert
//	fragColour.xyz += specular; // Specular
//
//	if (texture(diffuseTex, IN.texCoord).a != 1.0) {
//		fragColour = vec4(diffuse.xyz, 1.0);
//	}
//
//	//if (texture(diffuseTex, IN.texCoord).a < 0.95) {
//	//	fragColour = vec4(diffuse.xyz, 1.0);
//	//}
//
//	fragColour.a = 1.0;
//}