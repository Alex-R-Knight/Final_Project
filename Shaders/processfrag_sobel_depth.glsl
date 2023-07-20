#version 330 core

uniform sampler2D depthTex;

in Vertex{
vec2 texCoord;
} IN;

out vec4 fragColor;
out vec4 fragColor2;

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

	float measureX = 0.0f;
	float measureY = 0.0f;

	vec2 deltax = dFdx(IN.texCoord);
	vec2 deltay = dFdy(IN.texCoord);

	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			float sample = texture2D(depthTex, IN.texCoord.xy + (deltax * i, deltay * j).xy).r;
			measureX += (sample * sobelx[i][j]);
		}
	}

	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			float sample = texture2D(depthTex, IN.texCoord.xy + (deltax * i, deltay * j).xy).r;
			measureY += (sample * sobely[i][j]);
		}
	}

	//measureX = clamp(abs(measureX) + abs(measureY), 0.0f, 1.0f);

	fragColor = vec4(measureX, measureX, measureX, 1.0);

	// Debug
	float debugSample = (texture2D(depthTex, IN.texCoord.xy + (deltax, deltay).xy).r) * sobelx[2][2];

	fragColor2 = vec4(debugSample, debugSample, debugSample, 1.0f);

	//debug
	//float debugVal = texture2D(depthTex, IN.texCoord.xy).r;
	//
	//fragColor = vec4(debugVal, debugVal, debugVal, 1.0f);
}


//0.9, 0.8, 0.8
//0.9, 0.8, 0.8
//0.8, 0.7, 0.7
//
//mX = 0.1 + 0.2 + 0.1 = 0.4