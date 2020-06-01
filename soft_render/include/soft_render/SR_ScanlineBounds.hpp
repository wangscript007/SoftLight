
#ifndef SR_SCANLINEBOUNDS_HPP
#define SR_SCANLINEBOUNDS_HPP

#include <cstdint>

#include "lightsky/setup/Api.h"

#include "lightsky/math/vec2.h"
#include "lightsky/math/vec4.h"
#include "lightsky/math/scalar_utils.h"
#include "lightsky/math/vec_utils.h"

#include "soft_render/SR_Config.hpp"



/*-------------------------------------
 * Branchless vertex swap for SSE
-------------------------------------*/
#ifdef LS_ARCH_X86

inline LS_INLINE void sr_sort_minmax(__m128& a, __m128& b)  noexcept
{
    const __m128 ay    = _mm_permute_ps(a, 0x55);
    const __m128 by    = _mm_permute_ps(b, 0x55);
    const __m128 masks = _mm_cmplt_ps(ay, by);
    const __m128 at   = a;
    const __m128 bt   = b;
    a = _mm_blendv_ps(at, bt, masks);
    b = _mm_blendv_ps(bt, at, masks);
}

#elif defined(LS_ARCH_ARM)

inline LS_INLINE void sr_sort_minmax(float32x4_t& a, float32x4_t& b)  noexcept
{
    const float32x4_t ya   = vdupq_n_f32(vgetq_lane_f32(a, 1));
    const float32x4_t yb   = vdupq_n_f32(vgetq_lane_f32(b, 1));
    const uint32x4_t  ai   = vreinterpretq_u32_f32(a);
    const uint32x4_t  bi   = vreinterpretq_u32_f32(b);

    const uint32x4_t mask = vcltq_f32(ya, yb);
    const uint32x4_t al   = vandq_u32(vmvnq_u32(mask), ai);
    const uint32x4_t bg   = vandq_u32(vmvnq_u32(mask), bi);
    const uint32x4_t bl   = vandq_u32(mask, bi);
    const uint32x4_t ag   = vandq_u32(mask, ai);

    a = vreinterpretq_f32_u32(vorrq_u32(al, bl));
    b = vreinterpretq_f32_u32(vorrq_u32(ag, bg));
}

#endif



inline LS_INLINE void sr_sort_minmax(int32_t& a, int32_t& b)  noexcept
{
    #if 1 //!defined(LS_ARCH_X86)
        const int32_t mask = -(a >= b);
        const int32_t al   = ~mask & a;
        const int32_t bg   = ~mask & b;
        const int32_t ag   = mask & a;
        const int32_t bl   = mask & b;

        a = al | bl;
        b = ag | bg;
    #else
        const __m128i ai = _mm_set1_epi32(a);
        const __m128i bi = _mm_set1_epi32(b);
        const __m128i m  = _mm_cmplt_epi32(bi, ai);

        a = _mm_cvtsi128_si32(_mm_or_si128(_mm_and_si128(m, bi), _mm_andnot_si128(m, ai)));
        b = _mm_cvtsi128_si32(_mm_or_si128(_mm_and_si128(m, ai), _mm_andnot_si128(m, bi)));
    #endif
}



/*-----------------------------------------------------------------------------
 * Common method to get the beginning and end of a scanline.
-----------------------------------------------------------------------------*/
struct alignas(sizeof(float)*4) SR_ScanlineBounds
{
    ls::math::vec2 v0;
    ls::math::vec2 v1;

    float p20y;
    float p21xy;
    float p10xy;
    float p20x;

    inline void LS_INLINE init(ls::math::vec4 p0, ls::math::vec4 p1, ls::math::vec4 p2) noexcept
    {
        #if defined(LS_ARCH_X86) || defined(LS_ARCH_ARM)
            sr_sort_minmax(p0.simd, p1.simd);
            sr_sort_minmax(p0.simd, p2.simd);
            sr_sort_minmax(p1.simd, p2.simd);
        #else
            if (p0[1] < p1[1]) std::swap(p0, p1);
            if (p0[1] < p2[1]) std::swap(p0, p2);
            if (p1[1] < p2[1]) std::swap(p1, p2);
        #endif

        v0 = ls::math::vec2_cast(p0);
        v1 = ls::math::vec2_cast(p1);

        p20y  = 1.f / (p2[1] - p0[1]);
        p21xy = (p2[0] - p1[0]) / (p2[1] - p1[1]);
        p10xy = (p1[0] - p0[0]) / (p1[1] - p0[1]);
        p20x  = p2[0] - p0[0];
    }

    inline void LS_INLINE step(const float yf, int32_t& xMin, int32_t& xMax) const noexcept
    {
        const float d0 = yf - v0[1];
        const float d1 = yf - v1[1];

        const float alpha = d0 * p20y;

        #if defined(LS_ARCH_X86)
            const __m128i lo         = _mm_cvtps_epi32(_mm_set_ss(p20x * alpha + v0[0]));
            const __m128  secondHalf = _mm_cmplt_ss(_mm_set_ss(d1), _mm_setzero_ps());
            const __m128  a          = _mm_and_ps(secondHalf,    _mm_set_ss(p21xy * d1 + v1[0]));
            const __m128  b          = _mm_andnot_ps(secondHalf, _mm_set_ss(p10xy * d0 + v0[0]));
            const __m128i hi         = _mm_cvtps_epi32(_mm_or_ps(a, b));
            const __m128i m          = _mm_cmplt_epi32(hi, lo);

            xMin = _mm_cvtsi128_si32(_mm_or_si128(_mm_and_si128(m, hi), _mm_andnot_si128(m, lo)));
            xMax = _mm_cvtsi128_si32(_mm_or_si128(_mm_and_si128(m, lo), _mm_andnot_si128(m, hi)));
        #else
            const float lo         = ls::math::fmadd(p20x, alpha, v0[0]);
            const int   secondHalf = ls::math::sign_mask(d1);
            const float a          = ls::math::fmadd(p21xy, d1, v1[0]);
            const float b          = ls::math::fmadd(p10xy, d0, v0[0]);
            const float hi         = secondHalf ? a : b;

            xMin = (int32_t)lo;
            xMax = (int32_t)hi;
            sr_sort_minmax(xMin, xMax);
        #endif
    }
};


#endif /* SR_SCANLINEBOUNDS_HPP */
