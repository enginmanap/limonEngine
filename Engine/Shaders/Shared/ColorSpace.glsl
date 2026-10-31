#ifndef COLORSPACE_GLSL
#define COLORSPACE_GLSL

//every final write to the screen goes through this, a shader that skips it shows dark on a UNORM screen and nothing detects that
#ifdef LIMON_SCREEN_UNORM
vec3 linearToOutputColor(vec3 linearColor) {
    return pow(linearColor, vec3(1.0 / 2.2));
}
#else
vec3 linearToOutputColor(vec3 linearColor) {
    return linearColor;
}
#endif

#endif // COLORSPACE_GLSL