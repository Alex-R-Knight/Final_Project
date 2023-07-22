#version 330 core

uniform sampler2D depthTex;

uniform sampler2D normalTex;

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

	vec2 deltax = dFdx(IN.texCoord);
	vec2 deltay = dFdy(IN.texCoord);

	float kernelUL = texture2D(depthTex, IN.texCoord.xy - deltax + deltay).r;
	float kernelUM = texture2D(depthTex, IN.texCoord.xy - 0 + deltay).r;
	float kernelUR = texture2D(depthTex, IN.texCoord.xy + deltax + deltay).r;
	
	float kernelML = texture2D(depthTex, IN.texCoord.xy - deltax + 0).r;
	float kernelMR = texture2D(depthTex, IN.texCoord.xy + deltax + 0).r;

	float kernelLL = texture2D(depthTex, IN.texCoord.xy - deltax - deltay).r;
	float kernelLM = texture2D(depthTex, IN.texCoord.xy - 0 - deltay).r;
	float kernelLR = texture2D(depthTex, IN.texCoord.xy + deltax - deltay).r;

	float xValue = ( kernelUL + 2*kernelML + kernelLL ) - ( kernelUR + 2*kernelMR + kernelLR );

	float yValue = ( kernelUL + 2*kernelUM + kernelUR ) - ( kernelLL + 2*kernelLM + kernelLR );

	float magnitude = sqrt(xValue*xValue + yValue*yValue);

	float threshold = 0.02;

	fragColor = (magnitude > threshold) ? vec4(1.0) : vec4(0.0);
}