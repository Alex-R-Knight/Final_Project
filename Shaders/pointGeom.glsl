#version 330 core

uniform float particleSize = 10.0f;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

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
		OUT.colour = IN[i].colour;
		//top right
		gl_Position = gl_in[i].gl_Position;
		gl_Position.x += particleSize;
		gl_Position.y += particleSize;
		OUT.texCoord = vec2(1, 0);
		EmitVertex();
		//top left
		gl_Position = gl_in[i].gl_Position;
		gl_Position.x -= particleSize;
		gl_Position.y += particleSize;
		OUT.texCoord = vec2(0, 0);
		EmitVertex();
		// bottom right
		gl_Position = gl_in[i].gl_Position;
		gl_Position.x += particleSize;
		gl_Position.y -= particleSize;
		OUT.texCoord = vec2(1, 1);
		EmitVertex();
		// bottom Left
		gl_Position = gl_in[i].gl_Position;
		gl_Position.x -= particleSize;
		gl_Position.y -= particleSize;
		OUT.texCoord = vec2(0, 1);
		EmitVertex();

		EndPrimitive();
	}
}