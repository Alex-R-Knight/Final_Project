#version 330 core

uniform sampler2D depthTex;
uniform sampler2D normTex;

uniform vec2 pixelSize; // reciprocal of resolution
uniform vec3 cameraPos;

uniform float lightRadius;
uniform vec3 lightPos;
uniform vec4 lightColour;
uniform mat4 inverseProjView;

out vec4 diffuseOutput;
out vec4 specularOutput;

void main(void) {
	vec2 texCoord = vec2(gl_FragCoord.xy * pixelSize);	
	float depth = texture(depthTex, texCoord.xy).r;
	vec3 ndcPos = vec3(texCoord, depth) * 2.0 - 1.0;
	vec4 invClipPos = inverseProjView * vec4(ndcPos, 1.0);
	vec3 worldPos = invClipPos.xyz / invClipPos.w;

	float dist = length(lightPos - worldPos);

	if (dist > lightRadius) {
		discard;
	}

	//float atten = 1.0 / (dist * dist);

	//float atten = 1.0 - clamp ( dist / lightRadius , 0.0 , 1.0);
	// Attenuation testing


	float RadiusAtten = 1.0 - clamp( dist / lightRadius , 0.0 , 1.0);
	float invSqrAtten = lightColour.w / (dist*dist);

	float atten = clamp( RadiusAtten * invSqrAtten , 0.0 , 1.0);

	vec3 normal = normalize(texture(normTex, texCoord.xy).xyz * 2.0 - 1.0);
	vec3 incident = normalize(lightPos - worldPos);
	//vec3 viewDir = normalize(cameraPos - worldPos);
	//vec3 halfDir = normalize(incident + viewDir);

	float lambert = clamp(dot(incident, normal), 0.0, 1.0);
	//float specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
	//specFactor = pow(specFactor, 60.0);

	vec3 attenuated = lightColour.xyz * atten;
	diffuseOutput = vec4(attenuated * lambert, 1.0);

	diffuseOutput.x = ( diffuseOutput.x <= 0.2f ) ? diffuseOutput.x : 0.2f;
	diffuseOutput.y = ( diffuseOutput.y <= 0.2f ) ? diffuseOutput.y : 0.2f;
	diffuseOutput.z = ( diffuseOutput.z <= 0.2f ) ? diffuseOutput.z : 0.2f;



	//specularOutput = vec4(attenuated * specFactor * 0.33, 1.0);
	specularOutput = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}