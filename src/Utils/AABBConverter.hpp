//
// Created by engin on 04/03/2026.
//

#ifndef LIMONENGINE_AABBSCREENSPACECONVERTER_HPP
#define LIMONENGINE_AABBSCREENSPACECONVERTER_HPP
#include <glm/glm.hpp>
#include <algorithm>
#include <limits>
#include <glm/gtc/type_ptr.hpp>

// Detect headers based on architecture
#if defined(__SSE4_1__)
    #include <smmintrin.h>
#elif defined(__ARM_NEON)
    #include <arm_neon.h>
#endif

struct AABBConverter {
    static void getNCDAABB(const glm::vec3& worldMin, const glm::vec3& worldMax,
                           const glm::mat4& viewProj, glm::vec3& ndcMin, glm::vec3& ndcMax) {
    #if defined(__SSE4_1__)
        __m128 col0 = _mm_loadu_ps(&viewProj[0][0]);
        __m128 col1 = _mm_loadu_ps(&viewProj[1][0]);
        __m128 col2 = _mm_loadu_ps(&viewProj[2][0]);
        __m128 col3 = _mm_loadu_ps(&viewProj[3][0]);

        __m128 minX = _mm_set1_ps(worldMin.x);
        __m128 maxX = _mm_set1_ps(worldMax.x);
        __m128 minY = _mm_set1_ps(worldMin.y);
        __m128 maxY = _mm_set1_ps(worldMax.y);
        __m128 minZ = _mm_set1_ps(worldMin.z);
        __m128 maxZ = _mm_set1_ps(worldMax.z);

        // Precompute terms to save multiplications
        // We only need to multiply min/max of each axis by the corresponding column once
        __m128 mX0 = _mm_mul_ps(minX, col0);
        __m128 mX1 = _mm_mul_ps(maxX, col0);
        __m128 mY0 = _mm_mul_ps(minY, col1);
        __m128 mY1 = _mm_mul_ps(maxY, col1);
        __m128 mZ0 = _mm_mul_ps(minZ, col2);
        __m128 mZ1 = _mm_mul_ps(maxZ, col2);

        __m128 resMin = _mm_set1_ps(std::numeric_limits<float>::max());
        __m128 resMax = _mm_set1_ps(std::numeric_limits<float>::lowest());
        __m128 epsilon = _mm_set1_ps(0.0001f);

        // Helper macro to process a corner
        #define PROCESS_CORNER(TX, TY, TZ) \
            { \
                __m128 clip = _mm_add_ps(_mm_add_ps(TX, TY), _mm_add_ps(TZ, col3)); \
                __m128 w = _mm_shuffle_ps(clip, clip, _MM_SHUFFLE(3, 3, 3, 3)); \
                w = _mm_max_ps(w, epsilon); \
                __m128 ndc = _mm_div_ps(clip, w); \
                resMin = _mm_min_ps(resMin, ndc); \
                resMax = _mm_max_ps(resMax, ndc); \
            }

        PROCESS_CORNER(mX0, mY0, mZ0);
        PROCESS_CORNER(mX1, mY0, mZ0);
        PROCESS_CORNER(mX0, mY1, mZ0);
        PROCESS_CORNER(mX1, mY1, mZ0);
        PROCESS_CORNER(mX0, mY0, mZ1);
        PROCESS_CORNER(mX1, mY0, mZ1);
        PROCESS_CORNER(mX0, mY1, mZ1);
        PROCESS_CORNER(mX1, mY1, mZ1);

        #undef PROCESS_CORNER

        // Clamp to NDC space [-1, 1]
        resMin = _mm_max_ps(resMin, _mm_set1_ps(-1.0f));
        resMax = _mm_min_ps(resMax, _mm_set1_ps(1.0f));

        // SAFE STORE
        float tempMin[4], tempMax[4];
        _mm_storeu_ps(tempMin, resMin);
        _mm_storeu_ps(tempMax, resMax);
        ndcMin = glm::vec3(tempMin[0], tempMin[1], tempMin[2]);
        ndcMax = glm::vec3(tempMax[0], tempMax[1], tempMax[2]);

    #elif defined(__ARM_NEON)
        float32x4_t col0 = vld1q_f32(&viewProj[0][0]);
        float32x4_t col1 = vld1q_f32(&viewProj[1][0]);
        float32x4_t col2 = vld1q_f32(&viewProj[2][0]);
        float32x4_t col3 = vld1q_f32(&viewProj[3][0]);

        float32x4_t mX0 = vmulq_n_f32(col0, worldMin.x);
        float32x4_t mX1 = vmulq_n_f32(col0, worldMax.x);
        float32x4_t mY0 = vmulq_n_f32(col1, worldMin.y);
        float32x4_t mY1 = vmulq_n_f32(col1, worldMax.y);
        float32x4_t mZ0 = vmulq_n_f32(col2, worldMin.z);
        float32x4_t mZ1 = vmulq_n_f32(col2, worldMax.z);

        float32x4_t resMin = vdupq_n_f32(std::numeric_limits<float>::max());
        float32x4_t resMax = vdupq_n_f32(std::numeric_limits<float>::lowest());
        float32x4_t epsilon = vdupq_n_f32(0.0001f);

        #define PROCESS_CORNER_NEON(TX, TY, TZ) \
            { \
                float32x4_t clip = vaddq_f32(vaddq_f32(TX, TY), vaddq_f32(TZ, col3)); \
                float w = std::max(vgetq_lane_f32(clip, 3), 0.0001f); \
                float32x4_t ndc = vmulq_n_f32(clip, 1.0f / w); \
                resMin = vminq_f32(resMin, ndc); \
                resMax = vmaxq_f32(resMax, ndc); \
            }

        PROCESS_CORNER_NEON(mX0, mY0, mZ0);
        PROCESS_CORNER_NEON(mX1, mY0, mZ0);
        PROCESS_CORNER_NEON(mX0, mY1, mZ0);
        PROCESS_CORNER_NEON(mX1, mY1, mZ0);
        PROCESS_CORNER_NEON(mX0, mY0, mZ1);
        PROCESS_CORNER_NEON(mX1, mY0, mZ1);
        PROCESS_CORNER_NEON(mX0, mY1, mZ1);
        PROCESS_CORNER_NEON(mX1, mY1, mZ1);

        #undef PROCESS_CORNER_NEON

        float32x4_t nOne = vdupq_n_f32(-1.0f);
        float32x4_t pOne = vdupq_n_f32(1.0f);
        resMin = vmaxq_f32(resMin, nOne);
        resMax = vminq_f32(resMax, pOne);

        ndcMin = { vgetq_lane_f32(resMin, 0), vgetq_lane_f32(resMin, 1), vgetq_lane_f32(resMin, 2) };
        ndcMax = { vgetq_lane_f32(resMax, 0), vgetq_lane_f32(resMax, 1), vgetq_lane_f32(resMax, 2) };

    #else
        // Fallback
        ndcMin = glm::vec3(std::numeric_limits<float>::max());
        ndcMax = glm::vec3(std::numeric_limits<float>::lowest());

        for (int i = 0; i < 8; ++i) {
            glm::vec3 corner;
            corner.x = (i & 1) ? worldMax.x : worldMin.x;
            corner.y = (i & 2) ? worldMax.y : worldMin.y;
            corner.z = (i & 4) ? worldMax.z : worldMin.z;

            glm::vec4 clip = viewProj * glm::vec4(corner, 1.0f);
            float w = std::max(clip.w, 0.0001f);
            glm::vec3 ndc = glm::vec3(clip) / w;

            ndcMin = glm::min(ndcMin, ndc);
            ndcMax = glm::max(ndcMax, ndc);
        }
        ndcMin = glm::max(ndcMin, glm::vec3(-1.0f));
        ndcMax = glm::min(ndcMax, glm::vec3(1.0f));
    #endif
    }

    static void getWorldSpaceAABB(const glm::mat4& worldTransformMatrix, const glm::vec3& localMin, const glm::vec3& localMax, glm::vec3& worldMin, glm::vec3& worldMax) {
        const float* m = glm::value_ptr(worldTransformMatrix);

    #if defined(__SSE4_1__)
        __m128 wMin = _mm_setr_ps(m[12], m[13], m[14], 0.0f);
        __m128 wMax = wMin;

        for (int i = 0; i < 3; ++i) {
            __m128 col = _mm_loadu_ps(&m[i * 4]);
            __m128 lMin = _mm_set1_ps(localMin[i]);
            __m128 lMax = _mm_set1_ps(localMax[i]);

            __m128 a = _mm_mul_ps(col, lMin);
            __m128 b = _mm_mul_ps(col, lMax);

            wMin = _mm_add_ps(wMin, _mm_min_ps(a, b));
            wMax = _mm_add_ps(wMax, _mm_max_ps(a, b));
        }

        worldMin.x = _mm_cvtss_f32(wMin);
        worldMin.y = _mm_cvtss_f32(_mm_shuffle_ps(wMin, wMin, _MM_SHUFFLE(1, 1, 1, 1)));
        worldMin.z = _mm_cvtss_f32(_mm_shuffle_ps(wMin, wMin, _MM_SHUFFLE(2, 2, 2, 2)));

        worldMax.x = _mm_cvtss_f32(wMax);
        worldMax.y = _mm_cvtss_f32(_mm_shuffle_ps(wMax, wMax, _MM_SHUFFLE(1, 1, 1, 1)));
        worldMax.z = _mm_cvtss_f32(_mm_shuffle_ps(wMax, wMax, _MM_SHUFFLE(2, 2, 2, 2)));

    #elif defined(__ARM_NEON)
        float32x4_t wMin = { m[12], m[13], m[14], 0.0f };
        float32x4_t wMax = wMin;

        for (int i = 0; i < 3; ++i) {
            float32x4_t col = vld1q_f32(&m[i * 4]);
            float32x4_t lMin = vdupq_n_f32(localMin[i]);
            float32x4_t lMax = vdupq_n_f32(localMax[i]);

            float32x4_t a = vmulq_f32(col, lMin);
            float32x4_t b = vmulq_f32(col, lMax);

            wMin = vaddq_f32(wMin, vminq_f32(a, b));
            wMax = vaddq_f32(wMax, vmaxq_f32(a, b));
        }

        vst1_f32(&worldMin.x, vget_low_f32(wMin));
        vst1q_lane_f32(&worldMin.z, wMin, 2);

        vst1_f32(&worldMax.x, vget_low_f32(wMax));
        vst1q_lane_f32(&worldMax.z, wMax, 2);

    #else
        worldMin = glm::vec3(m[12], m[13], m[14]);
        worldMax = worldMin;

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                float a = m[i * 4 + j] * localMin[i];
                float b = m[i * 4 + j] * localMax[i];
                worldMin[j] += std::min(a, b);
                worldMax[j] += std::max(a, b);
            }
        }
    #endif
    }
};

#endif //LIMONENGINE_AABBSCREENSPACECONVERTER_HPP