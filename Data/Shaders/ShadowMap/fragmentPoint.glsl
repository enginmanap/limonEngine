#version 330 core
in vec4 FragPos;

uniform vec3 lightPosition;
uniform float farPlanePoint;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPosition);
    lightDistance = lightDistance / farPlanePoint;
    gl_FragDepth = lightDistance;
}  