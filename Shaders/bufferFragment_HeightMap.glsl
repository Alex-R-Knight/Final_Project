#version 330 core

uniform sampler2D diffuseTex; // Diffuse texture map
uniform sampler2D diffuseTex_2; // Diffuse texture map
uniform sampler2D bumpTex; //Bump map
uniform sampler2D bumpTex_2; //Bump map

in Vertex{
	vec3 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
} IN;

out vec4 fragColour[2]; //Our final outputted colours!

void main(void) {
	mat3 TBN = mat3(normalize(IN.tangent),
		normalize(IN.binormal),
		normalize(IN.normal));

	fragColour[0] = mix(texture2D(diffuseTex_2, IN.texCoord), texture2D(diffuseTex, IN.texCoord), pow(abs(IN.normal.y), 8));

	vec3 normal = mix(texture2D(bumpTex_2, IN.texCoord).rgb, texture2D(bumpTex, IN.texCoord).rgb, pow(abs(IN.normal.y), 8)) * 2.0 - 1.0;
	

	normal = normalize(TBN * normalize(normal));
	
	fragColour[1] = vec4(normal.xyz * 0.5 + 0.5, 1.0);
}