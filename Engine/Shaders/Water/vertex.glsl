
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>

layout (location = 1) in vec3 position;
layout (location = 2) in vec2 texCoord;

uniform sampler2D waterNoiseSampler;

// Primary swell: an explicit traveling sine wave. Noise drifting in a fixed direction - however
// smoothly sampled or layered - has no strong periodic silhouette for the eye to lock onto, so it
// reads as a texture sliding around rather than a wave rolling forward. A sine wave has an
// unambiguous, clearly-trackable moving crest, which is what actually sells "scrolling". This is the
// dominant motion; the noise octaves below only add chop/sparkle detail on top of it. It also has a
// simple closed-form derivative, which is what the flat per-triangle normal is computed from below.
const vec2  WAVE_DIRECTION = vec2(0.9284767, 0.3713907); // normalize(vec2(1.0, 0.4)) - the fixed "wind" direction the swell travels in
const float WAVE_PRIMARY_AMPLITUDE = 0.22;
const float WAVE_PRIMARY_WAVELENGTH = 18.0; // world units from crest to crest
const float WAVE_PRIMARY_SPEED = 2.2;       // world units/second the crest travels at

// Chop/detail: a sum of octaves (fractal Brownian motion) sampled from the SAME noise texture at
// geometrically increasing scale/speed, decreasing amplitude, and a different scroll direction each,
// layered on top of the primary swell above. This only feeds vertex HEIGHT (the mesh's bumpy
// silhouette), not the normal: the mesh is flat-shaded (one normal per triangle - see VS_FS below), so
// only whichever vertex GL picks as the provoking one would ever show a noise-driven normal, for no
// real visual benefit - the swell's own derivative is enough to shade the facets.
const int WAVE_OCTAVES = 3;
const float WAVE_BASE_AMPLITUDE = 0.09;  // height contribution of the first (biggest) chop layer
const float WAVE_PERSISTENCE = 0.4;      // amplitude multiplier applied per subsequent octave (< 1: each layer weaker)
const float WAVE_BASE_SCALE = 0.05;      // world-units -> UV for the first chop layer
const float WAVE_LACUNARITY = 2.6;       // UV-scale multiplier per subsequent octave (> 1: each layer tighter/finer)
const float WAVE_BASE_SPEED = 0.05;      // UV-units/second the first chop layer scrolls at
const float WAVE_SPEED_GROWTH = 1.3;     // scroll-speed multiplier per subsequent octave (finer layers drift quicker)
const float WAVE_ANGLE_STEP = 2.399963; // radians between each octave's scroll direction (golden angle - avoids repeating alignment)

out VS_FS {
    vec3 worldPos;
    flat vec3 normal; // flat = one normal per triangle (the provoking vertex's), not interpolated -
                       // this is what gives the low-poly faceted look instead of smooth shading across
                       // each triangle, which would hide the mesh's actual (low) resolution.
    vec2 texCoord;
} to_fs;

float waterHeight(vec2 worldXZ, float time) {
    float primaryFrequency = 6.28318530718 / WAVE_PRIMARY_WAVELENGTH;
    float primaryPhase = primaryFrequency * (dot(worldXZ, WAVE_DIRECTION) - WAVE_PRIMARY_SPEED * time);
    float height = sin(primaryPhase) * WAVE_PRIMARY_AMPLITUDE;

    float amplitude = WAVE_BASE_AMPLITUDE;
    float scale = WAVE_BASE_SCALE;
    float speed = WAVE_BASE_SPEED;
    float angle = 0.0;

    for (int i = 0; i < WAVE_OCTAVES; ++i) {
        vec2 direction = vec2(cos(angle), sin(angle));
        vec2 uv = worldXZ * scale + direction * (time * speed);
        float noiseValue = textureLod(waterNoiseSampler, uv, 0.0).r;
        height += (noiseValue - 0.5) * 2.0 * amplitude;

        amplitude *= WAVE_PERSISTENCE;
        scale *= WAVE_LACUNARITY;
        speed *= WAVE_SPEED_GROWTH;
        angle += WAVE_ANGLE_STEP;
    }
    return height;
}

void main(void) {
    // playerTransforms.time is engine milliseconds; converted to seconds for the scroll speeds above.
    float time = float(playerTransforms.time) * 0.001;

    float height = waterHeight(position.xz, time);

    // d/dx[sin(k * (dot(pos,dir) - speed*t))] = k * dir * cos(...) - the analytic derivative of the
    // primary swell only (see the comment above WAVE_OCTAVES for why the chop layers don't need this).
    float primaryFrequency = 6.28318530718 / WAVE_PRIMARY_WAVELENGTH;
    float primaryPhase = primaryFrequency * (dot(position.xz, WAVE_DIRECTION) - WAVE_PRIMARY_SPEED * time);
    vec2 slope = WAVE_DIRECTION * (cos(primaryPhase) * WAVE_PRIMARY_AMPLITUDE * primaryFrequency);

    to_fs.worldPos = vec3(position.x, position.y + height, position.z);
    to_fs.normal = normalize(vec3(-slope.x, 1.0, -slope.y));
    to_fs.texCoord = texCoord;

    gl_Position = playerTransforms.cameraProjection * vec4(to_fs.worldPos, 1.0);
}
