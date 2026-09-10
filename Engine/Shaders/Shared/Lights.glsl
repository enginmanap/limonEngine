// the only declaration of the light UBO. import this directly wherever LightSources is named,
// even if a shared helper already pulls it in - the guard collapses the duplicate.
// field order is hardcoded as byte offsets in OpenGLGraphics::setLight, reordering breaks it silently.
#define_option performance_maximumLights

#ifndef LIGHT_DEFINITIONS
#define LIGHT_DEFINITIONS
struct LightSource {
    //vec3 float, vec3 float is to make use of the std140 alignment, not mistake
    mat4 shadowMatrices[6];
    vec3 position;
    float radius;
    vec3 color;
    int type; //1 Directional, 2 point
    vec3 attenuation;
    float intensity;
    vec3 ambient;
    float falloffExponent;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[performance_maximumLights];
} LightSources;
#endif
