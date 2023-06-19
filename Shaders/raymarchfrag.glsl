#version 330 core


// Assorted notes here //
//
// dimensions of heightmap size, effectively current boundaries, are 11184 x 11184


in Vertex{
vec2 texCoord;
} IN;

out vec4 fragColor;


uniform mat4 lensProjection;

uniform sampler2D positionTexture;
uniform sampler2D directionTexture;

//// Raymarch Parameters ////

float maxDistance = 15;
float resolution = 0.3;
int steps = 10;
float thickness = 0.5;

/////////////////////////////




void main(void) {
	
	// Read texture size
	vec2 texSize  = textureSize(positionTexture, 0).xy;
	// texcoord generated if it wasnt already there
	//vec2 texCoord = gl_FragCoord.xy / texSize;

////// UV VEC4 to be used as shader output //////
	vec4 uv = vec4(0.0);
//////

	// Viewspace position of current fragment
	vec4 positionFrom     = texture(positionTexture, IN.texCoord);

	// Terminate if invalid
	if ( positionFrom.w <= 0.0 ) { fragColor = uv; return; }

	// Produces normalized vector of viewspace position
	vec3 unitPositionFrom = normalize(positionFrom.xyz);


	// For use in screenspace reflection ray directions
	//vec3 normal           = normalize(texture(normalTexture, IN.texCoord).xyz);
	//vec3 pivot            = normalize(reflect(unitPositionFrom, normal));

	// Unpacks and normalizes the ray firection for global illumination
	vec3 hemisphereVector = normalize( (texture(directionTexture, IN.texCoord).xyz - 0.5) * 2);

////// VEC4 to hold the actively read viewspace positions during raymarching ////// 
	vec4 positionTo = vec4(0);
//////

////// View space ray start and end position //////
	// Start position
	vec4 startView = vec4(positionFrom.xyz, 1);
	// End position of start pos plus maxdistance times normalized direction vector
	vec4 endView   = vec4(positionFrom.xyz + (hemisphereVector * maxDistance), 1);
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

	// UV coordinates to fragment coordinates
	startFrag.xy	*= texSize;

	//End position
	vec4 endFrag	= endView;

	// Multiply by projection matrix to screen space
	endFrag			= lensProjection * endFrag;

	// Perspective divide
	endFrag.xyz		/= endFrag.w;

	// Screen space X-Y coordinates to UV coordinates
	endFrag.xy		= endFrag.xy * 0.5 + 0.5;

	// UV coordinates to fragment coordinates
	endFrag.xy		*= texSize;

///////

/////// First Pass Preparation ///

	//Produce UV coordinate of fragment position
	vec2 frag  = startFrag.xy;
    
	// Divide fragment coordinates by texture size for UV coordinates
	uv.xy = frag / texSize;

	//X and Y delta values of the march across fragments
	float deltaX    = endFrag.x - startFrag.x;
	float deltaY    = endFrag.y - startFrag.y;

	//Check the greater delta value
	float useX      = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
	float delta     = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0, 1.0);

	// Divide deltas by greater delta, so largest axis movement is one
	vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001);

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
	float depth        = 0.0;


////// First pass //////
	for (float i = 0; i < int(delta); ++i) {

		// Frag is incremented by the per-step increment value
		frag		+= increment;

		// Divide fragment coordinates by texture size for UV coordinates
		uv.xy		= frag / texSize;

		// Reads the position info of the calculated UV coordinates
		positionTo	= texture(positionTexture, uv.xy);

		// Determines distance along the line, using the X or Y based on the useX determined above. Then clamp to 0-1 range.
		search1 = mix(
			(frag.y - startFrag.y) / deltaY,
			(frag.x - startFrag.x) / deltaX,
			useX
        );
		search1 = clamp(search1, 0.0, 1.0);

		// use search1 to interpolate (perspective-correctly) the viewspace position
		viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);

		// Calculate depth by comparing the ray and the fragment depths
		depth        = viewDistance - positionTo.z;

		// if an intersection is detected, hit0 is set to 1, and the first pass ends.
		if (depth > 0 && depth < thickness) {
		  hit0 = 1;
		  break;
		}
		// otherwise, search0 is set to search1, and the for loop repeats. This is to mark the current last known miss in the first pass.
		else {
		  search0 = search1;
		}
	}
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
		positionTo = texture(positionTexture, uv.xy);

		// use search1 to interpolate (perspective-correctly) the viewspace position
		viewDistance	= (startView.z * endView.z) / mix(endView.z, startView.z, search1);

		// Calculate depth by comparing the ray and the fragment depths
		depth        = viewDistance - positionTo.z;

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


////// Visibility calculation //////

	float visibility =
		// Has hit
		hit1

		// Hit position is readable
		* positionTo.w

		// Reduces as direction moves to face camera
		// using dot product of vector from position to camera, and ray direction vector
		* ( 1
			- max
			( dot(-unitPositionFrom, hemisphereVector),
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
			(   length(positionTo - positionFrom) / maxDistance,          
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

	//uv.rgb = texture2D(directionTexture, IN.texCoord.xy).xyz;

	fragColor = uv;
}