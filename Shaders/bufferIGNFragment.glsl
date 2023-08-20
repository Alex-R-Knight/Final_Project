#version 330 core

uniform sampler2D diffuseTex; // Diffuse texture map
uniform sampler2D bumpTex; //Bump map

// Reflect
uniform sampler2D reflectTex; //reflect map

in Vertex{
	vec3 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
	vec3 viewSpacePos;
} IN;

out vec4 fragColour[4]; //Our final outputted colours!

float ign(vec2 st)
{
    st = fract(st * vec2(5.3983, 5.4427));
    st += dot(st, st.yx + vec2(21.5351, 14.3137));
    return fract(st.x * st.y * (st.x + st.y));
}

void main(void) {
	if (texture(diffuseTex, IN.texCoord).a < 0.5) {
		discard;
	}
	mat3 TBN = mat3(normalize(IN.tangent),
		normalize(IN.binormal),
		normalize(IN.normal));

	vec3 normal = texture2D(bumpTex, IN.texCoord).rgb * 2.0 - 1.0;
	normal = normalize(TBN * normalize(normal));

	fragColour[0] = texture2D(diffuseTex, IN.texCoord);
	fragColour[0].a = 1.0;
	fragColour[1] = vec4(normal.xyz * 0.5 + 0.5, 1.0);

	//// Reflection

	fragColour[2] = texture2D(reflectTex, IN.texCoord);
}