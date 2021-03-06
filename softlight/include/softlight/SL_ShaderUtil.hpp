
#ifndef SL_SHADERUTIL_HPP
#define SL_SHADERUTIL_HPP

#include <atomic>
#include <cstdlib> // size_t

#include "lightsky/setup/Api.h"
#include "lightsky/setup/Types.h"

#include "lightsky/math/scalar_utils.h"
#include "lightsky/math/vec4.h"

#include "softlight/SL_ScanlineBounds.hpp"



/*-----------------------------------------------------------------------------
 * Helper Functions
-----------------------------------------------------------------------------*/
/**
 * @brief Calculate the optimal tiling for the fragment shader threads.
 *
 * Given a number of threads, retrieve the optimal number of horizontal and
 * vertical subdivisions to divide a framebuffer.
 *
 * @param numThreads
 * The number of threads which will be rendering to a framebuffer.
 *
 * @param numHoriz
 * Output parameter to hold the number of horizontal subdivisions for a
 * framebuffer.
 *
 * @param numVert
 * Output parameter to hold the number of vertical subdivisions for a
 * framebuffer.
 */
template <typename data_type>
inline void sl_calc_frag_tiles(data_type numThreads, data_type& numHoriz, data_type& numVert) noexcept
{
    // Create a set of horizontal and vertical tiles. This method will create
    // more horizontal tiles than vertical ones.
    data_type tileCount = ls::math::fast_sqrt<data_type>(numThreads);
    tileCount += (numThreads % tileCount) != 0;
    numHoriz  = ls::math::gcd(numThreads, tileCount);
    numVert   = numThreads / numHoriz;
}



/**
 * @brief Subdivide a rectangular region into equally spaced areas.
 *
 * @param w
 * The total width of the region to subdivide.
 *
 * @param h
 * The total height of the region to subdivide.
 *
 * @param numThreads
 * The number of threads to consider for subdivision.
 *
 * @param threadId
 * The current thread ID.
 *
 * @return A 4D vector, containing the following parameters for the current
 * thread, respectively:
 *     - Beginning X coordinate
 *     - Ending X coordinate
 *     - Beginning Y coordinate
 *     - Ending Y coordinate
 */
template <typename data_t>
inline ls::math::vec4_t<data_t> sl_subdivide_region(
    data_t w,
    data_t h,
    const data_t numThreads,
    const data_t threadId
) noexcept
{
    data_t cols;
    data_t rows;

    sl_calc_frag_tiles<data_t>(numThreads, cols, rows);
    w = w / cols;
    h = h / rows;

    const data_t x0 = w * (threadId % cols);
    const data_t y0 = h * ((threadId / cols) % rows);
    const data_t x1 = w + x0;
    const data_t y1 = h + y0;

    return ls::math::vec4_t<data_t>{x0, x1, y0, y1};
}



/**
 * @brief Calculate a shader processor's start/end positions
 *
 * @tparam vertsPerPrim
 * The number of vertices which specify a primitive (i.e. 2 for lines, 3 for
 * triangles).
 *
 * @tparam lastThreadProcessesLess
 * Boolean value to determine if the final thread will be given additional
 * elements to process, while all other threads contain an even distribution
 * of work. If this is false, the last thread will be given less elements to
 * process while the other threads are given a larger even distribution of
 * work to process.
 *
 * @param totalVerts
 * The total number of vertices to be processed.
 *
 * @param numThreads
 * The number of threads available to process vertices.
 *
 * @param threadId
 * The current thread index.
 *
 * @param outBegin
 * Output to store the beginning vertex (inclusive) to process for the current
 * thread.
 *
 * @param outEnd
 * Output to store the final vertex (exclusive) to process for the current
 * thread.
 */
template <size_t vertsPerPrim, bool lastThreadProcessesLess = true>
inline void sl_calc_indexed_parition(size_t totalVerts, size_t numThreads, size_t threadId, size_t& outBegin, size_t& outEnd) noexcept
{
    size_t totalPrims = totalVerts / vertsPerPrim;
    size_t activeThreads = numThreads < totalPrims ? numThreads : totalPrims;
    size_t chunkSize = totalVerts / activeThreads;
    size_t remainder = chunkSize % vertsPerPrim;
    size_t beg;
    size_t end;

    // Set to 0 for the last thread to share chunk processing, plus remainder.
    // Set to 1 for the last thread to only process remaining values.
    if (lastThreadProcessesLess)
    {
        chunkSize += vertsPerPrim - remainder;
    }
    else
    {
        chunkSize -= remainder;
    }

    beg = threadId * chunkSize;
    end = beg + chunkSize;

    if (threadId == (numThreads - 1))
    {
        end += totalVerts - (chunkSize * activeThreads);
    }

    outBegin = beg;
    outEnd = end < totalVerts ? end : totalVerts;
}

/**
 * @brief Calculate a shader processor's start/end positions
 *
 * @tparam vertsPerPrim
 * The number of vertices which specify a primitive (i.e. 2 for lines, 3 for
 * triangles).
 *
 * @param totalVerts
 * The total number of vertices to be processed.
 *
 * @param numThreads
 * The number of threads available to process vertices.
 *
 * @param threadId
 * The current thread index.
 *
 * @param outBegin
 * Output to store the beginning vertex (inclusive) to process for the current
 * thread.
 *
 * @param outEnd
 * Output to store the final vertex (exclusive) to process for the current
 * thread.
 */
template <size_t vertsPerPrim>
void sl_calc_indexed_parition2(size_t count, size_t numThreads, size_t threadId, size_t& outBegin, size_t& outEnd)
{
    const size_t totalRange  = (count / numThreads);
    const size_t threadRange = totalRange + (vertsPerPrim - (totalRange % 3));
    const size_t begin       = threadRange * threadId;
    const size_t end         = begin + threadRange;

    outBegin = begin;
    outEnd   = end < count ? end : count;
}



/*-----------------------------------------------------------------------------
 * Constants needed for shader operation
-----------------------------------------------------------------------------*/
enum SL_ShaderLimits
{
    SL_SHADER_MAX_WORLD_COORDS    = 3,
    SL_SHADER_MAX_SCREEN_COORDS   = 3,
    SL_SHADER_MAX_VARYING_VECTORS = 4,
    SL_SHADER_MAX_FRAG_OUTPUTS    = 4,

    // Maximum number of fragments that get queued before being placed on a
    // framebuffer.
    SL_SHADER_MAX_QUEUED_FRAGS    = 600,

    // Maximum number of vertex groups which get binned before being sent to a
    // fragment processor.
    SL_SHADER_MAX_BINNED_PRIMS    = 1024,
};




/*-----------------------------------------------------------------------------
 * Padded data types to avoid false sharing
-----------------------------------------------------------------------------*/
template <typename data_t>
struct alignas(sizeof(data_t)) SL_BinCounter
{
    typename ls::setup::EnableIf<ls::setup::IsIntegral<data_t>::value, data_t>::type count;
    uint8_t padding[128-sizeof(data_t)];
};



template <typename data_t>
struct alignas(sizeof(data_t)) SL_BinCounterAtomic
{
    std::atomic<typename ls::setup::EnableIf<ls::setup::IsIntegral<data_t>::value, data_t>::type> count;
    uint8_t padding[128-sizeof(data_t)];
};



/*-----------------------------------------------------------------------------
 * Intermediate Fragment Storage for Binning
-----------------------------------------------------------------------------*/
struct alignas(sizeof(ls::math::vec4)) SL_FragmentBin
{
    // 4-byte floats * 4-element vector * 3 vectors-per-tri = 48 bytes
    ls::math::vec4 mScreenCoords[SL_SHADER_MAX_SCREEN_COORDS];

    // 4-byte floats * 4-element vector * 3 barycentric coordinates = 48 bytes
    ls::math::vec4 mBarycentricCoords[SL_SHADER_MAX_SCREEN_COORDS];

    // 4-byte floats * 4-element vector * 3-vectors-per-tri * 4 varyings-per-vertex = 192 bytes
    ls::math::vec4 mVaryings[SL_SHADER_MAX_SCREEN_COORDS * SL_SHADER_MAX_VARYING_VECTORS];

    // 288 bytes = 2304 bits
};



/*-----------------------------------------------------------------------------
 * Helper structure to put a pixel on the screen
-----------------------------------------------------------------------------*/
struct alignas(alignof(uint64_t)) SL_FragCoordXYZ
{
    uint16_t x;
    uint16_t y;
    float depth;
};

struct SL_FragCoord
{
    ls::math::vec4 bc[SL_SHADER_MAX_QUEUED_FRAGS]; // 32*4

    SL_FragCoordXYZ coord[SL_SHADER_MAX_QUEUED_FRAGS];
    // 256 bits / 32 bytes
};



#endif /* SL_SHADERUTIL_HPP */
