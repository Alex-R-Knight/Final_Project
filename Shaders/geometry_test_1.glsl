#version 150 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in Vertex{
	vec4 colour;
	vec2 texCoord;
} IN[];

out Vertex{
	vec4 colour;
	vec2 texCoord;
} OUT;

void main(void) {
	for (int i = 0; i < gl_in.length(); ++i) {
		gl_Position = gl_in[i].gl_Position;

		OUT.colour = IN[i].colour;
		OUT.texCoord = IN[i].texCoord;
		EmitVertex();
	}
	EndPrimitive();
}