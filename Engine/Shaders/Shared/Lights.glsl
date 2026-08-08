// the only declaration of the light UBO. import this directly wherever LightSources is named,
// even if a shared helper already pulls it in - the guard collapses the duplicate.
// field order is hardcoded as byte offsets in OpenGLGraphics::setLight, reordering breaks it silently.
#define_option performance_maximumLights

#ifndef LIGHT_DEFINITIONS
#define LIGHT_DEFINITIONS
struct LightSource {
    mat4 shadowMatrices[6];
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type; //1 Directional, 2 point
    vec3 attenuation;
    vec3 ambient;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[performance_maximumLights];
} LightSources;
#endif
