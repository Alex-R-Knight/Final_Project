#version 330 core

uniform sampler2D diffuseTex; // Diffuse texture map
uniform sampler2D bumpTex; //Bump map

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


	////Stochastic Normal Generation////

	//// VERSION ONE - UNSURE OF RESULTS ////

	//	//Attempted Interleaved Gradient Noise
	//	//float noise = mod(52.9829189f * mod(0.06711056f*float(IN.texCoord.x) + 0.00583715f*float(IN.texCoord.y), 1.0f), 1.0f);
	//	float noise = 0.5f;
	//
	//	vec3 bitangent = normal;
	//	bitangent.x = -bitangent.x;
	//
	//	vec3 newTangent = cross(bitangent, normal);
	//
	//	float r = sqrt(noise);
	//
	//	float phi = 2.0f * 3.14159265f * noise;
	//
	//	vec3 weightedVector = newTangent * (r * cos(phi)) + bitangent * (r * sin(phi)) + normal.xyz * sqrt(max(0.0, 1.0f - noise));
	//
	//	//fragColour[2] = vec4(weightedVector.xyz, 1.0);
	//	fragColour[2] = vec4(weightedVector.xyz * 0.5 + 0.5, 1.0);


	//// VERSION TWO


	/// Raymarch Direction ///

	float rand1 = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
	float rand2 = fract(sin(dot(vec2(rand1, 0.0), vec2(12.9898, 78.233))) * 43758.5453);
	
	vec3 up = abs(normal.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 newTangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, newTangent);

	float theta = 2.0 * 3.14159 * rand1;
    float phi = acos(sqrt(rand2));

    vec3 sampleDir = vec3(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));

	sampleDir = newTangent * sampleDir.x + bitangent * sampleDir.y + normal * sampleDir.z;

	fragColour[2] = vec4(sampleDir.xyz * 0.5 + 0.5, 1.0);

	//// Viewspace Pos ////

	fragColour[3] = vec4(IN.viewSpacePos.xyz, 1.0);
}