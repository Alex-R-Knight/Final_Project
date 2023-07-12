#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 modelMatrix;

void main(void) {
	gl_Position = modelMatrix * vec4(position, 1.0);
}