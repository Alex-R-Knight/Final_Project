#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D diffuseLight;
uniform sampler2D specularLight;
uniform sampler2D rayMarchUV;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColour;

void main(void) {

	vec2 texSize	= textureSize(diffuseTex, 0).xy;
	vec2 newTexCoord	= gl_FragCoord.xy / texSize;

	vec4 uv			= texture(rayMarchUV,	IN.texCoord);
	vec4 color		= texture(diffuseTex,	uv.xy);

	//float alpha		= clamp(uv.b, 0, 1);
	//
	//fragColour		= vec4(mix(vec3(0), color.rgb, alpha), alpha);

	fragColour = ( uv.b != 0 ? color : vec4(0) );
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