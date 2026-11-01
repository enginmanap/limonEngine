#ifndef COLORSPACE_GLSL
#define COLORSPACE_GLSL

//every final write to the screen goes through this, a shader that skips it shows dark on a UNORM screen and nothing detects that
#if defined(LIMON_LINEAR_COLOR) && defined(LIMON_SCREEN_UNORM)
vec3 linearToOutputColor(vec3 linearColor) {
    return pow(linearColor, vec3(1.0 / 2.2));
}
#else
vec3 linearToOutputColor(vec3 linearColor) {
    return linearColor;
}
#endif

//every color image sample goes through this
#if defined(LIMON_LINEAR_COLOR) && defined(LIMON_TEXTURES_UNORM)
vec3 colorTextureToLinear(vec3 sampledColor) {
    return pow(sampledColor, vec3(2.2));
}
#else
vec3 colorTextureToLinear(vec3 sampledColor) {
    return sampledColor;
}
#endif

#endif // COLORSPACE_GLSL