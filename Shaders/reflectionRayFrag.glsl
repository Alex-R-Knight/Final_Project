#version 330 core


// Assorted notes here //
//
// dimensions of heightmap size, effectively current boundaries, are 11184 x 11184
//
// search for phrase "SWITCHHERE" for parts to flip between reflected rays or cosine weighted hemispheres

in Vertex{
vec2 texCoord;
} IN;

//out vec4 fragColor;

out vec4 uvOutput;
out vec4 debugOutput;
out vec4 debugOutput2;
out vec4 debugOutput3;
out vec4 debugOutput4;


uniform vec3 cameraPos;

uniform mat4 lensProjection;
uniform mat4 inverseProjection;
uniform mat4 NormalViewMatrix;
uniform mat4 InverseViewMatrix;

uniform vec2 pixelSize; // reciprocal of resolution

//uniform sampler2D positionTexture;
uniform sampler2D depthTex;

uniform sampler2D hemisphereTexture;
uniform sampler2D normalTexture;
//reflect
uniform sampler2D reflectionTexture;

// skybox
uniform samplerCube cubeTex;

// input colour
uniform sampler2D baseTexture;


//// Raymarch Parameters ////

float maxDistance = 50;
float resolution = 1.0;
int steps = 25;
const float thickness = 0.4;

/////////////////////////////


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

// Test numbers
// UV coord 0.75, 0.25, 0.90711

// NDC step
// 0.5, -0.5, 0.81422

// inverse projection
// 




void main(void) {

//float maxDistance = 50;
//float resolution = 1;
//int steps = 25;
//const float thickness = 0.1f;


////// UV vec4 to be used as shader output //////
	vec4 uv = vec4(0.0f, 0.0f, 0.0f, 0.0f);
//////
	vec2 texSize  = textureSize(hemisphereTexture, 0).xy;

	vec2 newTexCoord = vec2(gl_FragCoord.xy / texSize);
	// Viewspace position of current fragment
	vec4 positionFrom     = viewSpacePosFromDepth(IN.texCoord);

	// Terminate if invalid
	//if ( positionFrom.w <= 0.0 ) { fragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f); return; }
	if ( positionFrom.w <= 0.0 || texture(reflectionTexture, newTexCoord.xy).r <= 0.0 ) { uvOutput = vec4(0.0f, 0.0f, 0.0f, 0.0f); return; }

	// Produces normalized vector of viewspace position
	vec3 unitPositionFrom = normalize(positionFrom.xyz);


	// For use in screenspace reflection ray directions

	// Retrieve world space normal
	vec3 normal			= normalize( texture(normalTexture, newTexCoord.xy).xyz * 2.0 - 1.0 );
	// World space normal to view space
	normal				= mat3(NormalViewMatrix) * normal;

////// Debug storage
	// 
	//vec4 debugValue3 = vec4(normal.xyz, 1.0);
//////

	// Reflect normalised view space position via view space normal
	vec3 pivot			= normalize(reflect(unitPositionFrom, normal));

	// Unpacks and normalizes the ray direction for global illumination
	vec3 hemisphereVector = normalize( (texture(hemisphereTexture, newTexCoord.xy).xyz - 0.5) * 2);

////// VEC4 to hold the actively read viewspace positions during raymarching ////// 
	vec4 positionTo = vec4(0);
//////

////// View space ray start and end position //////
	// Start position
	vec4 startView = vec4(positionFrom.xyz, 1);

	// End position of start pos plus maxdistance times normalized direction vector
	//vec4 endView   = vec4(positionFrom.xyz + (hemisphereVector * maxDistance), 1);

	// reflection based alternate [SWITCHHERE]
	vec4 endView   = vec4(positionFrom.xyz + (maxDistance * pivot), 1);

////// The end View position must not go into positive Z axis space
	if (endView.z > 0) {

		float ratio = -startView.z / (endView.z - startView.z + 1);

		endView.xyz = startView.xyz + ratio * (endView.xyz - startView.xyz);
	}
//////

////// Debug storage
	vec4 debugValue = startView;
	vec4 debugValue2 = endView;
//////

///////

/////// Screen space translation of view space positions ///

	// Start position
	vec4 startFrag	= startView;

	// Multiply by projection matrix to screen space
	startFrag		= lensProjection * startFrag;

	// Perspective divide
	startFrag.xyz	/= startFrag.w;

	// Screen space X-Y coordinates to UV coordinates
	startFrag.xy	= startFrag.xy * 0.5 + 0.5;

////// Debug storage
	// Working as intended on renderdoc test
	//vec4 debugValue = vec4(startFrag.xy, 0.0, 1.0);
//////

	// UV coordinates to fragment coordinates
	startFrag.xy	*= texSize;

////// Debug storage
	// Working as intended, fragment coordinates output
	//vec4 debugValue3 = vec4(startFrag.xy, 0.0, 1.0);
//////

	//End position
	vec4 endFrag	= endView;

	// Multiply by projection matrix to screen space
	endFrag			= lensProjection * endFrag;


////// Debug storage
	// 
	//vec4 debugValue3 = vec4(endFrag.rgba);
//////

	// Perspective divide
	endFrag.xyz		/= endFrag.w;

////// Debug storage
	// 
	//vec4 debugValue3 = vec4(endFrag.xy, 0.0, 1.0);
//////

	// Screen space X-Y coordinates to UV coordinates //SUSSY
	endFrag.xy		= endFrag.xy * 0.5 + 0.5;

////// Debug storage
	// 
	//vec4 debugValue4 = vec4(endFrag.xy, 0.0, 1.0);
//////

	// UV coordinates to fragment coordinates
	endFrag.xy		*= texSize;


////// Restricting fragment coordinates to functional range
//	if (endFrag.x > texSize.x || endFrag.x < 0 || endFrag.y > texSize.y || endFrag.y < 0) {
//		
//		vec4 direction = vec4(endFrag.xy-startFrag.xy, 0, 0);
//
//		float xBorderDist = (direction.x > 0) ? (texSize.x - startFrag.x) : (startFrag.x);
//
//		float yBorderDist = (direction.y > 0) ? (texSize.y - startFrag.y) : (startFrag.y);
//
//		float xRealDist = abs(endFrag.x - startFrag.x);
//
//		float yRealDist = abs(endFrag.y - startFrag.y);
//
//		float xRatio = xBorderDist / xRealDist;
//
//		float yRatio = yBorderDist / yRealDist;
//
//		endFrag = (xRatio < yRatio) ? (startFrag + xRatio * direction) : (startFrag + yRatio * direction);
//	}
//////

////// Debug storage
	// 
	//vec4 debugValue4 = vec4(endFrag.xy, 0.0, 1.0);
//////



/////// First Pass Preparation ///

	//Produce UV coordinate of fragment position
	vec2 frag  = startFrag.xy;
    
////// Debug storage
	//// frag here outputs frag coords
	//vec4 debugValue3 = vec4(frag.xy, 0.0, 1.0);
//////

	// Divide fragment coordinates by texture size for UV coordinates
	uv.xy = frag / texSize;

////// Debug storage
	//// frag here outputs frag coords
	//vec4 debugValue4 = vec4(uv.xy, 0.0, 1.0);
//////

	//X and Y delta values of the march across fragments
	float deltaX    = endFrag.x - startFrag.x;
	float deltaY    = endFrag.y - startFrag.y;

	//Check the greater delta value
	float useX      = abs(deltaX) >= abs(deltaY) ? 1.0f : 0.0f;
	float delta     = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0f, 1.0f);

	// Divide deltas by greater delta, so largest axis movement is one
	vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001f);

////// Create search values
	// Search0 stores the last known position before an intersection. It is then used in the 2nd pass.
	float search0 = 0;

	// Search1 moves from 0 to 1, representing distance between start and end fragment
	float search1 = 0;
//////

////// Create hit values
	// Hit0 marks an intersection in the first pass
	int hit0 = 0;

	// Hit1 marks an intersection in the second pass
	int hit1 = 0;
//////

	// viewDistance stores current points distance from camera
	float viewDistance = startView.z;

	// Depth stores the distance difference between the ray point and the scene position from the position buffer
	float depth        = 0.0f;

////// Clamp value to prevent anomalous high delta
	float clampVal = (useX == 1.0f) ? texSize.x : texSize.y;
//////

////// Debug storage
	// outputs frag coordinates from "frag" now
	//vec4 debugValue = vec4(frag.xy, 0.0, 1.0);
//////


////// Clean debug setup
	vec4 debugValue3;
	vec4 debugValue4;
//////


////// First pass //////
	for (float i = 0; i <= clamp(delta, 0.0f, clampVal); ++i) {
	//for (float i = 0; i < 1; ++i) {
		
////////// Debug test
		debugValue = vec4(frag, 0, 1);
//////////


		// Frag is incremented by the per-step increment value
		frag		+= increment;

////////// Debug test
		debugValue2 = vec4(frag, 0, 1);
//////////

		// Divide fragment coordinates by texture size for UV coordinates
		uv.xy		= frag / texSize;

		// Reads the position info of the calculated UV coordinates
		positionTo	= viewSpacePosFromDepth(uv.xy);



		//Determines distance along the line, using the X or Y based on the useX determined above. Then clamp to 0-1 range.
		search1 = mix(
			(frag.y - startFrag.y) / deltaY,
			(frag.x - startFrag.x) / deltaX,
			useX
        );
		search1 = clamp(search1, 0.0, 1.0);

		//search1 = abs(frag.y - startFrag.y) / abs(deltaY);
		//search1 = clamp(search1, 0.0, 1.0);


		// use search1 to interpolate (perspective-correctly) the viewspace position
		viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);


		// viewDistance is the ray position
		// positionTo is the viewspace position of the geometry

		// Calculate depth by comparing the ray and the fragment depths
		depth        = positionTo.z- viewDistance;

////////// Debug test
		//debugValue3 = vec4(positionTo.xyz, 1);
//////////

////////// Debug test
		//debugValue4 = vec4(thickness, viewDistance, depth, 1);
//////////


		// if an intersection is detected, hit0 is set to 1, and the first pass ends.
		if (depth > 0 && depth < thickness) {
			//if ( abs(depth) < thickness ) {
			hit0 = 1;
			break;
		}
		// otherwise, search0 is set to search1, and the for loop repeats. This is to mark the current last known miss in the first pass.
		else {
		  search0 = search1;
		}
	}
//////

//////Debug work
	
	//uvOutput = vec4(uv.xy, hit0, 1);

	debugValue3 = vec4(uv.xy, hit0, 1);

//////

	// search1 is set to halfway between the last miss and the last hit positions
	search1 = search0 + ((search1 - search0) / 2.0);

	// If no intersection was found in the first pass, then changing steps to zero will prevent a second pass
	steps *= hit0;

////// Second pass
	for (float i = 0; i < steps; ++i) {
	
		// frag is set to the current search1 record of distance along the ray
		frag	= mix(startFrag.xy, endFrag.xy, search1);
	
		// the UV coordinates are generated as before from fragment coordinates and texture size
		uv.xy	= frag / texSize;
	
		// The position info of these UV coordinates are sampled
		positionTo = viewSpacePosFromDepth(uv.xy);
	
		// use search1 to interpolate (perspective-correctly) the viewspace position
		viewDistance	= (startView.z * endView.z) / mix(endView.z, startView.z, search1);
	
		// Calculate depth by comparing the ray and the fragment depths
		depth        = positionTo.z- viewDistance;
	
		// if an intersection is found, hit1 is set to 1, search1 is set to halfway between last known miss (search0) and the current hit
		if (depth > 0 && depth < thickness) {
			hit1 = 1;
			search1 = search0 + ((search1 - search0) / 2);
		}
		// If no intersection is found, search0 is set to the current miss position, search1 is set to halfway between the the last known hit, and the current miss
		else {
			float temp = search1;
			search1 = search1 + ((search1 - search0) / 2);
			search0 = temp;
		}
	}
//////

	debugValue4 = vec4(uv.xy, hit1, 1);

////// Visibility calculation //////

// Two versions for reflection or hemisphere sample [SWITCHHERE]

//	float visibility =
//		// Has hit
//		hit1
//
//		// Hit position is readable
//		* positionTo.w
//
//		// Reduces as direction moves to face camera
//		// using dot product of vector from position to camera, and ray direction vector
//		* ( 1
//			- max
//			( dot(-unitPositionFrom, hemisphereVector),
//			0
//			)
//		)
//
//		// Reduces as distance from intersection point increases
//		* ( 1
//			- clamp
//			( depth / thickness,
//			0,
//			1
//			)
//		)
//
//		// Reduces as distance from ray start point increases
//		* ( 1
//			- clamp
//			(   length(positionTo.xyz - positionFrom.xyz) / maxDistance,          
//			0,
//			1
//			)
//		)
//
//		// Removes is hit is beyond frustum area
//		* (uv.x < 0 || uv.x > 1 ? 0 : 1)
//		* (uv.y < 0 || uv.y > 1 ? 0 : 1);

float visibility =
		// Has hit
		hit1

		// Hit position is readable
		* positionTo.w

		// Reduces as direction moves to face camera
		// using dot product of vector from position to camera, and ray direction vector
		* ( 1
			- max
			( dot(-unitPositionFrom, pivot),
			0
			)
		)

		// Reduces as distance from intersection point increases
		* ( 1
			- clamp
			( depth / thickness,
			0,
			1
			)
		)

		// Reduces as distance from ray start point increases
		* ( 1
			- clamp
			(   length(positionTo.xyz - positionFrom.xyz) / maxDistance,          
			0,
			1
			)
		)

		// Removes is hit is beyond frustum area
		* (uv.x < 0 || uv.x > 1 ? 0 : 1)
		* (uv.y < 0 || uv.y > 1 ? 0 : 1);

	visibility = clamp(visibility, 0, 1);

	uv.b = visibility;


	//uv.r = 1.0;
	//uv.g = 0.0;
	//uv.b = 1.0;
	uv.a = 1.0;

	//uv.rgb = pivot.xyz * 0.5 + 0.5;

	//uv.rgb = texture2D(hemisphereTexture, IN.texCoord.xy).xyz;

	//uv.rgb = unitPositionFrom.rgb;

	// Defauly viability value
	//uv.b = (hit1 == 1.0) ? 1.0f : 0.0f;
	
	vec4 worldPosTemp = InverseViewMatrix * vec4(positionFrom.xyz, 1.0);

	vec3 worldPos = worldPosTemp.xyz / worldPosTemp.w;


	vec3 worldViewDir = normalize(cameraPos - worldPos);

	vec3 cubeMapReflectDirection = reflect( -worldViewDir, normalize( texture(normalTexture, newTexCoord.xy).xyz * 2.0 - 1.0 ) );


	
	// Output actual reflected colour to buffer
	if ( uv.b <= 0.0 ) {	
		uvOutput = texture( cubeTex, cubeMapReflectDirection );
	}
	else {
		uvOutput = texture( baseTexture, uv.xy);
	}
	

	// quick visibility test
	debugValue2 = vec4(uv.b, 0, 0, 1);

	// output UV coords with "value" in B component
	//uvOutput = vec4(uv.xyz, 1.0);

	// Debug buffers
	debugOutput = debugValue;
	debugOutput2 = debugValue2;
	debugOutput3 = debugValue3;
	debugOutput4 = debugValue4;
}