
#ifndef SR_FRAGMENT_PROCESSOR_HPP
#define SR_FRAGMENT_PROCESSOR_HPP

#include <atomic>
#include "lightsky/math/vec4.h"

#include "soft_render/SR_Mesh.hpp"

#include "soft_render/SR_Framebuffer.hpp"
#include "soft_render/SR_Shader.hpp"
#include "soft_render/SR_UniformBuffer.hpp"



/*-----------------------------------------------------------------------------
 * Forward Declarations
-----------------------------------------------------------------------------*/
namespace ls
{
    namespace utils
    {
        template<class WorkerTaskType>
        class Worker;
    } // end utils namespace
} // end ls namespace

struct SR_FragCoord; // SR_ShaderProcessor.hpp
struct SR_FragmentBin; // SR_ShaderProcessor.hpp
struct SR_ShaderProcessor;



/*-----------------------------------------------------------------------------
 * Helper Functions
-----------------------------------------------------------------------------*/
/**
 * @brief Retrieve the offset to a threads first renderable scanline.
 *
 * @tparam data_t
 * The requested data type.
 *
 * @param numThreads
 * The number of threads which are currently being used for rendering.
 *
 * @param threadId
 * The current thread's ID (0-based index).
 *
 * @param fragmentY
 * The initial scanline for a line or triangle being rendered.
 */
template <typename data_t>
constexpr data_t sr_scanline_offset(
    const data_t numThreads,
    const data_t threadId,
    const data_t fragmentY)
{
    return numThreads - 1 - (((fragmentY % numThreads) + threadId) % numThreads);
}



/*-----------------------------------------------------------------------------
 * Encapsulation of fragment processing on another thread.
 *
 * Point rasterization will divide the output framebuffer into equal parts,
 * so all threads will be assigned a specific region of the screen.
 *
 * Line rasterization will
-----------------------------------------------------------------------------*/
struct SR_FragmentProcessor
{
    // 32 bits
    uint16_t mThreadId;
    SR_RenderMode mMode;

    // 32-bits
    uint32_t mNumProcessors;

    // 64-bits
    float mFboW;
    float mFboH;

    // 64 bits
    uint_fast64_t mNumBins;

    // 256 bits
    const SR_Shader* mShader;
    SR_Framebuffer* mFbo;
    SR_FragmentBin* mBins;
    SR_FragCoord* mQueues;

    // 448 bits = 56 bytes

    void render_point(
        const uint_fast64_t binId,
        SR_Framebuffer* const fbo,
        const ls::math::vec4_t<int32_t> dimens) noexcept;

    void render_line(
        const uint_fast64_t binId,
        SR_Framebuffer* const fbo,
        const ls::math::vec4_t<int32_t> dimens) noexcept;

    void render_triangle(const uint_fast64_t binId, const SR_Texture* depthBuffer) const noexcept;

    void flush_fragments(const uint_fast64_t binId, uint32_t numQueuedFrags, const SR_FragCoord* outCoords) const noexcept;

    void execute() noexcept;
};



#endif /* SR_FRAGMENT_PROCESSOR_HPP */
