#version 330 core

// Screen space shadow version

//// The Plan:

// For each pixel that passes shadowmap testing,
// Raymarch from viewspace position of fragment towards light source
// In event of collision, pixel is occluded
// Also unoccluded pixels can receive a little occlusion based on normal compared to ray direction

////



// Sampler Textures
uniform sampler2D depthTex;
uniform sampler2D normTex;

// Shadowmaps
uniform sampler2D shadowTex1;
uniform sampler2D shadowTex2;
uniform sampler2D shadowTex3;
uniform sampler2D shadowTex4;
uniform sampler2D shadowTex5;
uniform sampler2D shadowTex6;

uniform mat4 shadowMatrix1;
uniform mat4 shadowMatrix2;
uniform mat4 shadowMatrix3;
uniform mat4 shadowMatrix4;
uniform mat4 shadowMatrix5;
uniform mat4 shadowMatrix6;

uniform vec2 pixelSize; // reciprocal of resolution
uniform vec3 cameraPos;

// Light Properties
uniform float lightRadius;
uniform vec3 lightPos;
uniform vec4 lightColour;

//uniform vec3 lightViewSpacePos;

// Transformation Matrices
uniform mat4 inverseProjView;
uniform mat4 inverseProjection;
uniform mat4 projMatrix;

uniform mat4 viewMatrix;

//// Output Information
out vec4 diffuseOutput;
out vec4 specularOutput;

// Debug
out vec4 debugOutput;
out vec4 debugOutput2;
out vec4 debugOutput3;
out vec4 debugOutput4;
////


//// Raymarch Parameters ////

const float maxDistance = 5;
const int steps = 1000;
const float thickness = 0.3;

/////////////////////////////

//// View Space Function
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
////

void main(void) {

	vec2 texCoord = vec2(gl_FragCoord.xy * pixelSize);


	float depth = texture(depthTex, texCoord.xy).r;
	vec3 ndcPos = vec3(texCoord, depth) * 2.0 - 1.0;
	vec4 invClipPos = inverseProjView * vec4(ndcPos, 1.0);
	vec3 worldPos = invClipPos.xyz / invClipPos.w;

	float dist = length(lightPos - worldPos);
	
	//float atten = 1.0 - clamp ( dist / lightRadius , 0.0 , 1.0);
	// Attenuation testing


	float RadiusAtten = 1.0 - clamp( dist / lightRadius , 0.0 , 1.0);
	float invSqrAtten = lightColour.w / (dist*dist);

	float atten = clamp( RadiusAtten * invSqrAtten , 0.0 , 1.0);

	if (atten == 0.0) {
		discard;
	}

	vec3 normal = normalize(texture(normTex, texCoord.xy).xyz * 2.0 - 1.0);
	vec3 incident = normalize(lightPos - worldPos);
	vec3 viewDir = normalize(cameraPos - worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	float lambert = clamp(dot(incident, normal), 0.0, 1.0);
	//float rFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
	float specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0);
	vec3 attenuated = lightColour.xyz * atten;

////// Pre-Shadow Output Values

	diffuseOutput = vec4(attenuated * lambert, 1.0);
	specularOutput = vec4(attenuated * specFactor * 0.33, 1.0);

//////


////// Shadowmapping hell

	vec3 shadowViewDir = normalize( lightPos - worldPos.xyz );

	// default pushval multiplied by only dot product
	// reduced pushval due to noticeable offset between surfaces due to large normal difference
	vec4 pushVal = vec4( normal , 0) * dot( shadowViewDir , normal ) * 0.5f;
	//vec4 pushVal = vec4( 0.0f, 0.0f, 0.0f, 0.0f );


	float shadow = 1.0f;


	// Oh boy its carpal tunnel time
	float kernalVal = 1.0f / 9.0f;

	vec2 texSize  = textureSize(shadowTex1, 0).xy;
	vec2 fragIncrements = vec2(1/texSize.x, 1/texSize.y);


////// RUN 1
	vec4 shadowProj = shadowMatrix1 * ( vec4(worldPos, 1) + pushVal );

	vec3 shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC.y ) < 1.0f &&
		abs ( shadowNDC.z ) < 1.0f )
	{
		vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		for (int y = -1; y < 2; y++)
		{
			for (int x = -1; x < 2; x++)
			{
				vec2 sampleCoord = (biasCoord.xy + vec2(x*fragIncrements.x, y*fragIncrements.y));

				float shadowZ = texture ( shadowTex1 , sampleCoord.xy ).x;

				if( shadowZ < biasCoord.z ) {
					 shadow -= kernalVal;
				}
			}
		}
	}
//////

////// RUN 2
	shadowProj = shadowMatrix2 * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		 vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		for (int y = -1; y < 2; y++)
		{
			for (int x = -1; x < 2; x++)
			{
				vec2 sampleCoord = (biasCoord.xy + vec2(x*fragIncrements.x, y*fragIncrements.y));

				float shadowZ = texture ( shadowTex2 , sampleCoord.xy ).x;

				if( shadowZ < biasCoord.z ) {
					 shadow -= kernalVal;
				}
			}
		}
	}
//////

////// RUN 3
	shadowProj = shadowMatrix3 * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		for (int y = -1; y < 2; y++)
		{
			for (int x = -1; x < 2; x++)
			{
				vec2 sampleCoord = (biasCoord.xy + vec2(x*fragIncrements.x, y*fragIncrements.y));

				float shadowZ = texture ( shadowTex3 , sampleCoord.xy ).x;

				if( shadowZ < biasCoord.z ) {
					 shadow -= kernalVal;
				}
			}
		}
	}
//////

////// RUN 4
	shadowProj = shadowMatrix4 * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		for (int y = -1; y < 2; y++)
		{
			for (int x = -1; x < 2; x++)
			{
				vec2 sampleCoord = (biasCoord.xy + vec2(x*fragIncrements.x, y*fragIncrements.y));

				float shadowZ = texture ( shadowTex4 , sampleCoord.xy ).x;

				if( shadowZ < biasCoord.z ) {
					 shadow -= kernalVal;
				}
			}
		}
	}
//////

////// RUN 5
	shadowProj = shadowMatrix5 * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		for (int y = -1; y < 2; y++)
		{
			for (int x = -1; x < 2; x++)
			{
				vec2 sampleCoord = (biasCoord.xy + vec2(x*fragIncrements.x, y*fragIncrements.y));

				float shadowZ = texture ( shadowTex5 , sampleCoord.xy ).x;

				if( shadowZ < biasCoord.z ) {
					 shadow -= kernalVal;
				}
			}
		}
	}
//////

////// RUN 6
	shadowProj = shadowMatrix6 * ( vec4(worldPos, 1) + pushVal );

	shadowNDC = shadowProj.xyz / shadowProj.w;
	if( abs ( shadowNDC.x ) < 1.0f &&
		abs ( shadowNDC . y ) < 1.0f &&
		abs ( shadowNDC . z ) < 1.0f )
	{
		vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		for (int y = -1; y < 2; y++)
		{
			for (int x = -1; x < 2; x++)
			{
				vec2 sampleCoord = (biasCoord.xy + vec2(x*fragIncrements.x, y*fragIncrements.y));

				float shadowZ = texture ( shadowTex6 , sampleCoord.xy ).x;

				if( shadowZ < biasCoord.z ) {
					 shadow -= kernalVal;
				}
			}
		}
	}
//////

if (shadow != 0.0f)
{
////// Screen Space Shadows

	// For use only on areas unoccluded by shadowmaps

	// Total fragment dimensions
	vec2 fragDivider  = textureSize(normTex, 0).xy;

	// Get View Space position of light
	vec3 lightViewSpacePos = (viewMatrix * vec4(lightPos.xyz, 1.0f)).xyz;

	// View Space Normal
	vec3 viewSpaceNormal = mat3(viewMatrix) * normal;

	//// Ray start position (view space from texcoord)
	//vec3 rayStartPosition = viewSpacePosFromDepth(texCoord.xy).xyz;

	// Ray start position (view space from texcoord)	WITH NORMAL OFFSET
	vec3 rayStartPosition = viewSpacePosFromDepth(texCoord.xy).xyz + viewSpaceNormal*0.05;

	// Vector to the light source
	vec3 lightDistanceVec3 = lightViewSpacePos-rayStartPosition;

	// Ray end position, accounting for max distance
	vec3 rayEndPosition = (lightDistanceVec3.length() < maxDistance) ? lightViewSpacePos : rayStartPosition + (maxDistance * normalize(lightDistanceVec3));

	// Prevent end position surpassing Z boundary
	if (rayEndPosition.z > 0)
	{
		float ratio = -rayStartPosition.z / (rayEndPosition.z - rayStartPosition.z + 1);

		rayEndPosition.xyz = rayStartPosition.xyz + ratio * (rayEndPosition.xyz - rayStartPosition.xyz);
	}

////// Start and end positions to fragment coordinates
	vec4 startFrag = vec4(rayStartPosition.xyz, 1);

	// Multiply by projection matrix to screen space
	startFrag	= projMatrix * startFrag;

	// Perspective divide
	startFrag.xyz	/= startFrag.w;

	// Screen space X-Y coordinates to UV coordinates
	startFrag.xy	= startFrag.xy * 0.5 + 0.5;

	// UV coordinates to fragment coordinates
	startFrag.xy	*= fragDivider;

	debugOutput = vec4(startFrag.xy, 0.0f, 1.0f);


	// End position
	vec4 endFrag = vec4(rayEndPosition.xyz, 1);

	// Multiply by projection matrix to screen space
	endFrag	= projMatrix * endFrag;

	// Perspective divide
	endFrag.xyz	/= endFrag.w;

	// Screen space X-Y coordinates to UV coordinates
	endFrag.xy	= endFrag.xy * 0.5 + 0.5;

	// UV coordinates to fragment coordinates
	endFrag.xy	*= fragDivider;

	debugOutput2 = vec4(endFrag.xy, 0.0f, 1.0f);
//////

	//Produce UV coordinate of fragment position
	vec2 frag  = startFrag.xy;

	//X and Y delta values of the march across fragments
	float deltaX    = endFrag.x - startFrag.x;
	float deltaY    = endFrag.y - startFrag.y;

	//Check the greater delta value
	float useX      = abs(deltaX) >= abs(deltaY) ? 1.0f : 0.0f;
	float delta     = mix(abs(deltaY), abs(deltaX), useX);

	// Set delta to smallest of given step count or delta distance
	delta = min(delta, steps);

	// Divide deltas by greater delta, so largest axis movement is one
	vec2  increment = vec2(deltaX, deltaY) / delta;

	debugOutput3 = vec4(increment.x, increment.y, delta, 1.0f);

	vec2 uv = vec2(0.0f);

	vec4 currentPosition;

	// Extra increment to stop close cuts
	//frag	+= increment;

	for (int i = 0; i < delta; i++)
	//for (int i = 0; i < 1; i++)
	{
		// Frag is incremented by the per-step increment value
		frag	+= increment;

		// Test if within boundaries
		if (frag.x > fragDivider.x || frag.x < 0 || frag.y > fragDivider.y || frag.y < 0)
		{
			// passed beyond screen space
			break;
		}

		// Divide fragment coordinates by texture size for UV coordinates
		uv.xy	= frag / fragDivider;

		// Reads the position info of the calculated UV coordinates
		currentPosition	= viewSpacePosFromDepth(uv.xy);

		//Determines distance along the line, using the X or Y based on the useX determined above. Then clamp to 0-1 range.
		float rayProgress = mix(
			(frag.y - startFrag.y) / deltaY,
			(frag.x - startFrag.x) / deltaX,
			useX
		);
		rayProgress = clamp(rayProgress, 0.0, 1.0);

		// use search1 to interpolate (perspective-correctly) the viewspace position
		float viewDistance = (rayStartPosition.z * rayEndPosition.z) / mix(rayEndPosition.z, rayStartPosition.z, rayProgress);

		// viewDistance is the ray position
		// currentPosition is the viewspace position of the geometry

		// Calculate depth by comparing the ray and the fragment depths
		depth	= currentPosition.z - viewDistance;

		// if an intersection is detected, fragment is occluded, break cycle
		if (depth > 0 && depth < thickness) {
			shadow = 0.0f;
			break;
		}
	}
//////
}


////// Final shader output

	diffuseOutput.rgb *= shadow;

	specularOutput.rgb *= shadow;

//////
}