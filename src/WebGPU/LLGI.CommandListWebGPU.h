#pragma once

/**
 * LLGI CommandListWebGPU - WebGPU Command List Implementation
 * 
 * Wraps WGPUCommandEncoder for recording GPU commands.
 */

#include "../LLGI.CommandList.h"
#include "LLGI.BaseWebGPU.h"

namespace LLGI
{

class GraphicsWebGPU;
class SingleFrameMemoryPoolWebGPU;
class RenderPassWebGPU;
class PipelineStateWebGPU;
class BufferWebGPU;
class TextureWebGPU;

/**
 * WebGPU Command List
 * 
 * Records GPU commands via WGPUCommandEncoder:
 * - Render pass begin/end
 * - Draw calls
 * - Resource binding
 * - Texture/buffer copies
 */
class CommandListWebGPU : public CommandList
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    SingleFrameMemoryPoolWebGPU* memoryPool_ = nullptr;
    
    WGPUCommandEncoder encoder_ = nullptr;
    WGPUCommandBuffer commandBuffer_ = nullptr;
    WGPURenderPassEncoder renderPassEncoder_ = nullptr;
    WGPUComputePassEncoder computePassEncoder_ = nullptr;
    
    // Current state for draw calls
    RenderPassWebGPU* currentRenderPass_ = nullptr;
    PipelineStateWebGPU* currentPipeline_ = nullptr;
    
    // Cached bind groups for efficiency
    WGPUBindGroup currentBindGroup_ = nullptr;
    bool bindGroupDirty_ = true;
    
    // Track current viewport/scissor
    struct {
        float x = 0, y = 0, w = 0, h = 0;
        float minDepth = 0, maxDepth = 1;
    } viewport_;
    
    struct {
        int32_t x = 0, y = 0, w = 0, h = 0;
    } scissor_;
    
    bool hasScissor_ = false;
    
    // External handles (when using platform integration)
    bool isExternalEncoder_ = false;
    bool isExternalRenderPass_ = false;

    // Helper methods
    void CreateBindGroup();
    void FlushBindings();

public:
    CommandListWebGPU(GraphicsWebGPU* graphics);
    ~CommandListWebGPU() override;
    
    bool Initialize(SingleFrameMemoryPoolWebGPU* memoryPool);
    
    // ========== CommandList Interface ==========
    
    void Begin() override;
    bool BeginWithPlatform(void* platformContextPtr) override;
    void End() override;
    void EndWithPlatform() override;
    
    void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void Draw(int32_t primitiveCount, int32_t instanceCount = 1) override;
    
    void SetVertexBuffer(Buffer* vertexBuffer, int32_t stride, int32_t offset) override;
    void SetIndexBuffer(Buffer* indexBuffer, int32_t stride, int32_t offset = 0) override;
    void SetPipelineState(PipelineState* pipelineState) override;
    void SetConstantBuffer(Buffer* constantBuffer, int32_t unit) override;
    void SetTexture(Texture* texture, TextureWrapMode wrapMode, TextureMinMagFilter minmagFilter, int32_t unit) override;
    void SetComputeBuffer(Buffer* computeBuffer, int32_t stride, int32_t unit, bool is_readonly) override;
    
    void CopyTexture(Texture* src, Texture* dst) override;
    void CopyTexture(Texture* src, Texture* dst, const Vec3I& srcPos, const Vec3I& dstPos, 
                     const Vec3I& size, int srcLayer, int dstLayer) override;
    
    void GenerateMipMap(Texture* src) override;
    
    void BeginRenderPass(RenderPass* renderPass) override;
    void EndRenderPass() override;
    bool BeginRenderPassWithPlatformPtr(void* platformPtr) override;
    bool EndRenderPassWithPlatformPtr() override;
    
    void BeginComputePass() override;
    void EndComputePass() override;
    void Dispatch(int32_t groupX, int32_t groupY, int32_t groupZ, 
                  int32_t threadX, int32_t threadY, int32_t threadZ) override;
    
    void CopyBuffer(Buffer* src, Buffer* dst) override;
    void SetImageData2D(Texture* texture, int32_t x, int32_t y, 
                        int32_t width, int32_t height, const void* data) override;
    
    void WaitUntilCompleted() override;
    
    // ========== WebGPU-Specific ==========
    
    WGPUCommandBuffer GetCommandBuffer() const { return commandBuffer_; }
    WGPUCommandEncoder GetEncoder() const { return encoder_; }
    WGPURenderPassEncoder GetRenderPassEncoder() const { return renderPassEncoder_; }
};

} // namespace LLGI
