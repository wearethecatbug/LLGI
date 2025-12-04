#pragma once

/**
 * LLGI GraphicsWebGPU - WebGPU Graphics Device Implementation
 * 
 * Provides WebGPU backend for LLGI, enabling Effekseer rendering on:
 * - Native platforms via Dawn (Windows, macOS, Linux)
 * - Web browsers via Emscripten + emdawnwebgpu
 */

#include "../LLGI.Graphics.h"
#include "LLGI.BaseWebGPU.h"

#include <functional>
#include <unordered_map>
#include <memory>

namespace LLGI
{

class RenderPassWebGPU;
class RenderPassPipelineStateWebGPU;
class TextureWebGPU;
class BufferWebGPU;
class CommandListWebGPU;
class SingleFrameMemoryPoolWebGPU;

/**
 * WebGPU Graphics Device
 * 
 * Wraps WGPUDevice and provides LLGI interface for:
 * - Buffer creation (vertex, index, uniform, storage)
 * - Texture creation (2D, render targets, depth)
 * - Shader loading (WGSL)
 * - Pipeline state creation
 * - Command list management
 * - Render pass management
 */
class GraphicsWebGPU : public Graphics
{
private:
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
    
    int32_t swapBufferCount_ = 0;
    
    // Pipeline state cache (keyed by render pass configuration)
    std::unordered_map<RenderPassPipelineStateKey, 
                       std::shared_ptr<RenderPassPipelineStateWebGPU>, 
                       RenderPassPipelineStateKey::Hash> renderPassPipelineStates_;
    
    // Owner reference (prevents premature destruction)
    ReferenceObject* owner_ = nullptr;
    
    // Callback for when GPU work completes (used in WaitFinish)
    bool workCompleted_ = true;

public:
    /**
     * Create GraphicsWebGPU
     * 
     * @param device WebGPU device handle
     * @param queue WebGPU queue handle
     * @param swapBufferCount Number of swap chain buffers (typically 2-3)
     * @param owner Optional owner object to prevent premature destruction
     */
    GraphicsWebGPU(WGPUDevice device,
                   WGPUQueue queue,
                   int32_t swapBufferCount,
                   ReferenceObject* owner = nullptr);
    
    ~GraphicsWebGPU() override;

    // ========== Core Graphics Interface ==========
    
    /**
     * Execute a command list
     * Submits command buffer to GPU queue
     */
    void Execute(CommandList* commandList) override;
    
    /**
     * Wait for all GPU work to complete
     * Blocks until queue is idle
     */
    void WaitFinish() override;

    // ========== Resource Creation ==========
    
    /**
     * Create a buffer (vertex, index, uniform, storage)
     */
    Buffer* CreateBuffer(BufferUsageType usage, int32_t size) override;
    
    /**
     * Create shader from WGSL source
     * @param data Array of DataStructure containing shader source
     * @param count Number of shader stages (vertex + fragment = 2)
     */
    Shader* CreateShader(DataStructure* data, int32_t count) override;
    
    /**
     * Create pipeline state object
     */
    PipelineState* CreatePiplineState() override;
    
    /**
     * Create single-frame memory pool for constant buffers
     */
    SingleFrameMemoryPool* CreateSingleFrameMemoryPool(int32_t constantBufferPoolSize, int32_t drawingCount) override;
    
    /**
     * Create command list for recording GPU commands
     */
    CommandList* CreateCommandList(SingleFrameMemoryPool* memoryPool) override;

    // ========== Render Pass Creation ==========
    
    /**
     * Create render pass with multiple render targets
     */
    RenderPass* CreateRenderPass(Texture** textures, int32_t textureCount, Texture* depthTexture) override;
    
    /**
     * Create render pass with MSAA resolve targets
     */
    RenderPass* CreateRenderPass(Texture* texture, Texture* resolvedTexture, 
                                 Texture* depthTexture, Texture* resolvedDepthTexture) override;

    // ========== Texture Creation ==========
    
    /**
     * Create texture with full parameter control
     */
    Texture* CreateTexture(const TextureParameter& parameter) override;
    
    /**
     * Create texture from external handle (for swapchain integration)
     */
    Texture* CreateTexture(uint64_t id) override;
    
    /**
     * Create standard texture for sampling
     */
    Texture* CreateTexture(const TextureInitializationParameter& parameter) override;
    
    /**
     * Create render target texture
     */
    Texture* CreateRenderTexture(const RenderTextureInitializationParameter& parameter) override;
    
    /**
     * Create depth/stencil texture
     */
    Texture* CreateDepthTexture(const DepthTextureInitializationParameter& parameter) override;

    // ========== Pipeline State Cache ==========
    
    /**
     * Create or retrieve cached render pass pipeline state
     */
    RenderPassPipelineState* CreateRenderPassPipelineState(RenderPass* renderpass) override;
    
    /**
     * Create render pass pipeline state from key
     */
    RenderPassPipelineState* CreateRenderPassPipelineState(const RenderPassPipelineStateKey& key) override;

    // ========== Debug & Utility ==========
    
    /**
     * Capture render target contents for debugging
     */
    std::vector<uint8_t> CaptureRenderTarget(Texture* renderTarget) override;
    
    /**
     * Create GPU query object
     */
    Query* CreateQuery(QueryType queryType, int32_t queryCount) override;
    
    /**
     * Convert timestamp to microseconds
     */
    uint64_t TimestampToMicroseconds(uint64_t timestamp) const override;

    // ========== WebGPU-Specific Accessors ==========
    
    WGPUDevice GetDevice() const { return device_; }
    WGPUQueue GetQueue() const { return queue_; }
    int32_t GetSwapBufferCount() const { return swapBufferCount_; }
    
    /**
     * Create a WGPUBuffer with specified usage
     * Helper for internal resource creation
     */
    WGPUBuffer CreateWGPUBuffer(WGPUBufferUsageFlags usage, uint64_t size, bool mappedAtCreation = false);
    
    /**
     * Create a WGPUTexture
     * Helper for internal resource creation
     */
    WGPUTexture CreateWGPUTexture(const WGPUTextureDescriptor& descriptor);
    
    /**
     * Create a WGPUSampler
     */
    WGPUSampler CreateSampler(WGPUAddressMode addressMode, WGPUFilterMode filterMode);
};

} // namespace LLGI
