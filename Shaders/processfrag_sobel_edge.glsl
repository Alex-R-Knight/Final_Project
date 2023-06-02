#version 330 core

uniform sampler2D sceneTex;

uniform int isVertical;

in Vertex{
vec2 texCoord;
} IN;

out vec4 fragColor;

mat3 sobelx = mat3(
	1.0, 2.0, 1.0,
	0.0, 0.0, 0.0,
	-1.0, -2.0, -1.0
);

mat3 sobely = mat3(
	1.0, 0.0, -1.0,
	2.0, 0.0, -2.0,
	1.0, 0.0, -1.0
);

void main(void) {
	
	vec3 diffuse = texture(sceneTex, IN.texCoord.xy).rgb;

	float measure = 0.0f;

	mat3 switcher;

	vec2 deltax = dFdx(IN.texCoord);
	vec2 deltay = dFdy(IN.texCoord);
	
	if (isVertical == 1) {
		switcher = sobely;
	}
	else {
		switcher = sobelx;
	}

	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			vec3 sample = texture2D(sceneTex, IN.texCoord.xy + (deltax * i, deltay * j).xy).rgb;
			measure += (length(sample) * switcher[i][j]);
		}
	}
	fragColor = vec4(diffuse * length(measure), 1.0);
}


//#version 330 core
//
//uniform sampler2D sceneTex;
//
//uniform int isVertical;
//
//in Vertex{
//vec2 texCoord;
//} IN;
//
//out vec4 fragColor;
//
//mat3 sobelx = mat3(
//	1.0, 2.0, 1.0,
//	0.0, 0.0, 0.0,
//	-1.0, -2.0, -1.0
//);
//
//mat3 sobely = mat3(
//	1.0, 0.0, -1.0,
//	2.0, 0.0, -2.0,
//	1.0, 0.0, -1.0
//);
//
//void main(void) {
//
//	vec3 diffuse = texture(sceneTex, IN.texCoord.xy).rgb;
//
//	float measure = 0.0f;
//
//	mat3 switcher;
//
//	vec2 deltax = dFdx(IN.texCoord);
//	vec2 deltay = dFdy(IN.texCoord);
//
//	if (isVertical == 1) {
//		switcher = sobely;
//	}
//	else {
//		switcher = sobelx;
//	}
//
//	for (int i = -1; i < 2; i++) {
//		for (int j = -1; j < 2; j++) {
//			vec3 sample = texture2D(sceneTex, IN.texCoord.xy + (deltax * i, deltay * j).xy).rgb;
//			measure += (length(sample) * switcher[i][j]);
//		}
//	}
//	fragColor = vec4(diffuse * length(measure), 1.0);
//}