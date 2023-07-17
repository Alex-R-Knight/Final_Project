# version 330 core

in Vertex{
vec2 texCoord;
} IN;

out vec4 fragColour;

uniform mat4 lensProjection;
uniform mat4 inverseProjection;

uniform sampler2D depthTex;
uniform sampler2D normalTex;
uniform sampler2D noiseTex;

uniform vec3 samples[32];

int kernelSize = 32;
//float radius = 0.5f;
float radius = 0.5f;
float bias = 0.0125;


vec4 viewSpacePosFromDepth(vec2 inCoord) {
	
	float depth = texture(depthTex, inCoord).r;

	// NDC position, X and Y mapped to -1 to 1 range along with Z
	vec3 ndcPos = vec3(inCoord.x, inCoord.y, depth) * 2.0 - 1.0;

	// Multiply by inverse projection matrix, expand to vec4 to do so
	vec4 invClipPos = inverseProjection * vec4(ndcPos, 1.0);

	// Divide by W value, perspective division process
	vec3 viewPos = invClipPos.xyz / invClipPos.w;

	vec4 returnVec = (depth >= 1.0) ? vec4(viewPos.xyz, 0.0f) : vec4(viewPos.xyz, 1.0);

	return returnVec;
}

void main ( void ) {
	vec4 positionFrom = viewSpacePosFromDepth(IN.texCoord);
	if ( positionFrom.w <= 0.0 ) { fragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f); return; }


	vec2 texSize  = textureSize(normalTex, 0).xy;
	vec2 noiseScale = texSize / 4.0f;


	vec3 normal = texture(normalTex, IN.texCoord).xyz * 2.0 - 1.0;
	vec3 noise = texture(noiseTex, IN.texCoord * noiseScale).xyz;
	
	// Toggle lines to use noise
	vec3 tangent   = normalize(noise - normal * dot(noise, normal));
	//vec3 tangent   = normalize(vec3(0, 0, 1) - normal * dot(vec3(0, 0, 1), normal));
	
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN       = mat3(tangent, bitangent, normal); 


	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
		vec3 samplePosition = TBN * samples[i];
		samplePosition = positionFrom.xyz + samplePosition * radius;

		// viewspace position to UV
		vec4 offset	= vec4(samplePosition, 1.0);
		offset		= lensProjection * offset; 
		offset.xyz /= offset.w;
		offset.xyz  = offset.xyz * 0.5 + 0.5;

		vec4 kernelSample = viewSpacePosFromDepth(offset.xy);

		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(positionFrom.z - kernelSample.z));

		occlusion += ( (kernelSample.z >= samplePosition.z + bias) ? 1.0 : 0.0 ) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / kernelSize);

	fragColour = vec4 (occlusion, occlusion, occlusion, 1.0);
}