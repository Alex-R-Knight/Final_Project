#version 330 core

uniform sampler2D depthTex;
uniform sampler2D normTex;

// Shadowmaps
uniform sampler2D shadowTex1;
uniform sampler2D shadowTex2;
uniform sampler2D shadowTex3;
uniform sampler2D shadowTex4;
uniform sampler2D shadowTex5;
uniform sampler2D shadowTex6;

uniform mat4 shadowMatrix[6];

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
	float atten = 1.0 - clamp ( dist / lightRadius , 0.0 , 1.0);

	if (atten == 0.0) {
		discard;
	}

	vec3 normal = normalize(texture(normTex, texCoord.xy).xyz * 2.0 - 1.0);
	vec3 incident = normalize(lightPos - worldPos);
	vec3 viewDir = normalize(cameraPos - worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	float lambert = clamp(dot(incident, normal), 0.0, 1.0);
	float rFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
	float specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0);
	vec3 attenuated = lightColour.xyz * atten;
	diffuseOutput = vec4(attenuated * lambert, 1.0);
	specularOutput = vec4(attenuated * specFactor * 0.33, 1.0);


	// Shadowmapping hell

	vec3 shadowViewDir = normalize( lightPos - worldPos.xyz );

	vec4 pushVal = vec4( normal , 0) * dot( shadowViewDir , normal );


	float shadow = 1.0;


	// Oh boy its carpal tunnel time


////// RUN 1
	vec4 shadowProj = shadowMatrix[0] * ( vec4(worldPos, 1) + pushVal );

	vec3 shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		 vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		 float shadowZ = texture ( shadowTex1 , biasCoord.xy ).x;

		 if( shadowZ < biasCoord.z ) {
			 shadow = 0.0f;
		 }
	}
//////

////// RUN 2
	shadowProj = shadowMatrix[1] * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		 vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		 float shadowZ = texture ( shadowTex2 , biasCoord.xy ).x;

		 if( shadowZ < biasCoord.z ) {
			 shadow = 0.0f;
		 }
	}
//////

////// RUN 3
	shadowProj = shadowMatrix[2] * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		 vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		 float shadowZ = texture ( shadowTex3 , biasCoord.xy ).x;

		 if( shadowZ < biasCoord.z ) {
			 shadow = 0.0f;
		 }
	}
//////

////// RUN 4
	shadowProj = shadowMatrix[3] * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		 vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		 float shadowZ = texture ( shadowTex4 , biasCoord.xy ).x;

		 if( shadowZ < biasCoord.z ) {
			 shadow = 0.0f;
		 }
	}
//////

////// RUN 5
	shadowProj = shadowMatrix[4] * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		 vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		 float shadowZ = texture ( shadowTex5 , biasCoord.xy ).x;

		 if( shadowZ < biasCoord.z ) {
			 shadow = 0.0f;
		 }
	}
//////

////// RUN 6
	shadowProj = shadowMatrix[5] * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		 vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		 float shadowZ = texture ( shadowTex6 , biasCoord.xy ).x;

		 if( shadowZ < biasCoord.z ) {
			 shadow = 0.0f;
		 }
	}
//////


	diffuseOutput.rgb *= shadow;

	specularOutput.rgb *= shadow;
}











//void main(void) {
//	vec2 texCoord = vec2(gl_FragCoord.xy * pixelSize);	
//	float depth = texture(depthTex, texCoord.xy).r;
//	vec3 ndcPos = vec3(texCoord, depth) * 2.0 - 1.0;
//	vec4 invClipPos = inverseProjView * vec4(ndcPos, 1.0);
//	vec3 worldPos = invClipPos.xyz / invClipPos.w;
//
//	float dist = length(lightPos - worldPos);
//
//	if (dist > lightRadius) {
//		discard;
//	}
//
//	float atten = 1.0 / (dist * dist);
//	//float atten = 1.0 / ((dist * dist) / 1000);
//
//	vec3 normal = normalize(texture(normTex, texCoord.xy).xyz * 2.0 - 1.0);
//	vec3 incident = normalize(lightPos - worldPos);
//	vec3 viewDir = normalize(cameraPos - worldPos);
//	vec3 halfDir = normalize(incident + viewDir);
//
//	float lambert = clamp(dot(incident, normal), 0.0, 1.0);
//	float rFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
//	float specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
//	specFactor = pow(specFactor, 60.0);
//	vec3 attenuated = lightColour.xyz * clamp(atten * lightColour.a, 0.0, 1.0);
//	diffuseOutput = vec4(attenuated * lambert, 1.0);
//	specularOutput = vec4(attenuated * specFactor * 0.33, 1.0);
//}