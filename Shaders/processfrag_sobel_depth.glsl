#version 330 core

uniform sampler2D depthTex;

uniform sampler2D normalTex;

//Toggle me if idea is failure
uniform sampler2D lightColourTex;

in Vertex{
vec2 texCoord;
} IN;

out vec4 fragColor;
out vec4 fragColor2;


void main(void) {

	vec2 deltax = dFdx(IN.texCoord);
	vec2 deltay = dFdy(IN.texCoord);

	// Depth
	float kernelUL = texture2D(depthTex, IN.texCoord.xy - deltax + deltay).r;
	float kernelUM = texture2D(depthTex, IN.texCoord.xy - 0 + deltay).r;
	float kernelUR = texture2D(depthTex, IN.texCoord.xy + deltax + deltay).r;
	
	float kernelML = texture2D(depthTex, IN.texCoord.xy - deltax + 0).r;
	float kernelMR = texture2D(depthTex, IN.texCoord.xy + deltax + 0).r;

	float kernelLL = texture2D(depthTex, IN.texCoord.xy - deltax - deltay).r;
	float kernelLM = texture2D(depthTex, IN.texCoord.xy - 0 - deltay).r;
	float kernelLR = texture2D(depthTex, IN.texCoord.xy + deltax - deltay).r;

	// Normal
	vec3 normalUL = texture2D(normalTex, IN.texCoord.xy - deltax + deltay).xyz * 2.0 - 1.0;
	vec3 normalUM = texture2D(normalTex, IN.texCoord.xy - 0 + deltay).xyz * 2.0 - 1.0;
	vec3 normalUR = texture2D(normalTex, IN.texCoord.xy + deltax + deltay).xyz * 2.0 - 1.0;

	vec3 normalML = texture2D(normalTex, IN.texCoord.xy - deltax + 0).xyz * 2.0 - 1.0;
	vec3 normalMR = texture2D(normalTex, IN.texCoord.xy + deltax + 0).xyz * 2.0 - 1.0;

	vec3 normalLL = texture2D(normalTex, IN.texCoord.xy - deltax - deltay).xyz * 2.0 - 1.0;
	vec3 normalLM = texture2D(normalTex, IN.texCoord.xy - 0 - deltay).xyz * 2.0 - 1.0;
	vec3 normalLR = texture2D(normalTex, IN.texCoord.xy + deltax - deltay).xyz * 2.0 - 1.0;


	// Depth
	float xValue = ( kernelUL + 2*kernelML + kernelLL ) - ( kernelUR + 2*kernelMR + kernelLR );

	float yValue = ( kernelUL + 2*kernelUM + kernelUR ) - ( kernelLL + 2*kernelLM + kernelLR );

	float magnitude = sqrt(xValue*xValue + yValue*yValue);

	float depthThreshold = 0.02;

	// Normal
	vec3 xGradient = (( normalUL + 2*normalML + normalLL ) - ( normalUR + 2*normalMR + normalLR ));

	vec3 yGradient = (( normalUL + 2*normalUM + normalUR ) - ( normalLL + 2*normalLM + normalLR ));

	float combinedGradient = dot(normalize(xGradient), normalize(yGradient));

	float normalThreshold = 1.0;

	float xGradientLength = length(xGradient);
	float yGradientLength = length(yGradient);

	// Lighting check
	float light = length(texture2D(lightColourTex, IN.texCoord.xy).xyz);

	vec4 lightvec4 = texture2D(lightColourTex, IN.texCoord.xy);

	// Final result
	fragColor = (magnitude > depthThreshold || xGradientLength > normalThreshold || yGradientLength > normalThreshold) ? vec4(1.0, light, 0.0f, 1.0f) : vec4(0.0);

	fragColor2 = lightvec4;
}