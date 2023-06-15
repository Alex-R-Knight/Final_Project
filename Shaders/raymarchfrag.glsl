#version 330 core





in Vertex{
vec2 texCoord;
} IN;

out vec4 fragColor;

//// Raymarch Parameters ////

float maxDistance = 15;
float resolution = 0.3;
int steps = 10;
float thickness = 0.5;

/////////////////////////////

uniform mat4 lensProjection;

uniform sampler2D positionTexture;
uniform sampler2D directionTexture;



void main(void) {
	
	vec2 texSize  = textureSize(positionTexture, 0).xy;
	vec2 texCoord = gl_FragCoord.xy / texSize;

	vec4 uv = vec4(0.0);

	vec4 positionFrom     = texture(positionTexture, texCoord);

	//Terminate if invalid
	if ( positionFrom.w <= 0.0 ) { fragColor = uv; return; }

	vec3 unitPositionFrom = normalize(positionFrom.xyz);

	//vec3 normal           = normalize(texture(normalTexture, texCoord).xyz);
	//vec3 pivot            = normalize(reflect(unitPositionFrom, normal));

	vec3 hemisphereVector = normalize( (texture(directionTexture, texCoord).xyz - 0.5) * 2);

	vec4 positionTo = positionFrom;

/////// View space ray start and end position ///

	vec4 startView = vec4(positionFrom.xyz + (hemisphereVector *           0), 1);
	vec4 endView   = vec4(positionFrom.xyz + (hemisphereVector * maxDistance), 1);

///////

/////// Screen space translation of view space positions ///

	//Start position
	vec4 startFrag	= startView;

	startFrag		= lensProjection * startFrag;

	startFrag.xyz	/= startFrag.w;

	startFrag.xy	= startFrag.xy * 0.5 + 0.5;

	startFrag.xy	*= texSize;

	//End position
	vec4 endFrag	= endView;

	endFrag			= lensProjection * endFrag;

	endFrag.xyz		/= endFrag.w;

	endFrag.xy		= endFrag.xy * 0.5 + 0.5;

	endFrag.xy		*= texSize;

///////

/////// First Pass Steps ///

	//Produce UV coordinate of fragment position
	vec2 frag  = startFrag.xy;
    
	uv.xy = frag / texSize;

	//X and Y delta values
	float deltaX    = endFrag.x - startFrag.x;
	float deltaY    = endFrag.y - startFrag.y;

	//Check the greater delta value
	float useX      = abs(deltaX) >= abs(deltaY) ? 1 : 0;
	float delta     = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0, 1);

	vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001);

	float search0 = 0;
	float search1 = 0;

	int hit0 = 0;
	int hit1 = 0;

	float viewDistance = startView.z;
	float depth        = thickness;


	// First pass
	for (int i = 0; i < int(delta); ++i) {

		frag      += increment;
		uv.xy      = frag / texSize;
		positionTo = texture(positionTexture, uv.xy);

		search1 = mix(
			(frag.y - startFrag.y) / deltaY,
			(frag.x - startFrag.x) / deltaX,
			useX
        );
		search1 = clamp(search1, 0.0, 1.0);

		viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
		depth        = viewDistance - positionTo.z;

		if (depth > 0 && depth < thickness) {
		  hit0 = 1;
		  break;
		}
		else {
		  search0 = search1;
		}
	}

	search1 = search0 + ((search1 - search0) / 2.0);

	steps *= hit0;

	// Second pass
	for (int i = 0; i < steps; ++i) {

		frag	= mix(startFrag.xy, endFrag.xy, search1);
		uv.xy	= frag / texSize;
		positionTo = texture(positionTexture, uv.xy);

		viewDistance	= (startView.z * endView.z) / mix(endView.z, startView.z, search1);
		depth        = viewDistance - positionTo.z;

		if (depth > 0 && depth < thickness) {
			hit1 = 1;
			search1 = search0 + ((search1 - search0) / 2);
		}
		else {
			float temp = search1;
			search1 = search1 + ((search1 - search0) / 2);
			search0 = temp;
		}
	}

	/// Visibility calculation ///

	float visibility =
		// Has hit
		hit1

		// Hit position is readable
		* positionTo.w

		// Reduces as direction moves to face camera
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

	fragColor = uv;
}