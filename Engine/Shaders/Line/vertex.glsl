#version 330

layout (location = 0) in int index;

uniform mat4 cameraTransformMatrix;
uniform mat4 lineInfo;

out vec4 vs_fs_color;

void main(void)
{
	if(index == 0) {
		gl_Position = cameraTransformMatrix * lineInfo[0];
		vs_fs_color = lineInfo[2];
	} else {
		gl_Position = cameraTransformMatrix * lineInfo[1];
		vs_fs_color = lineInfo[3];
	}
}
