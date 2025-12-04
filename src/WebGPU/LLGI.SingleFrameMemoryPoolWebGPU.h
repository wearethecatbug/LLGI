#pragma once

/**
 * LLGI SingleFrameMemoryPoolWebGPU - Per-frame memory allocator for WebGPU
 */

#include "../LLGI.SingleFrameMemoryPool.h"
#include "LLGI.BaseWebGPU.h"

#include <vector>

namespace LLGI
{

class GraphicsWebGPU;
class BufferWebGPU;

/**
 * Memory allocation result
 */
struct MemoryAllocationWebGPU
{
    WGPUBuffer buffer = nullptr;
    int32_t offset = 0;
    int32_t size = 0;
};

/**
 * WebGPU Single Frame Memory Pool
 * 
 * Manages per-frame constant buffer allocations.
 * Uses a ring buffer approach with multiple frames in flight.
 */
class SingleFrameMemoryPoolWebGPU : public SingleFrameMemoryPool
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    
    // Ring buffer for uniform data
    WGPUBuffer uniformBuffer_ = nullptr;
    int32_t uniformBufferSize_ = 0;
    int32_t uniformBufferOffset_ = 0;
    
    // Staging data for CPU writes
    std::vector<uint8_t> stagingData_;
    
    // Frame tracking
    int32_t currentFrame_ = 0;
    int32_t swapCount_ = 0;
    
    // Per-frame offsets for ring buffer
    std::vector<int32_t> frameOffsets_;
    
    // Constant buffer alignment (WebGPU requires 256 bytes)
    static constexpr int32_t UNIFORM_ALIGNMENT = 256;

public:
    SingleFrameMemoryPoolWebGPU(GraphicsWebGPU* graphics, 
                                 int32_t constantBufferPoolSize, 
                                 int32_t drawingCount);
    ~SingleFrameMemoryPoolWebGPU() override;
    
    // ========== SingleFrameMemoryPool Interface ==========
    
    void NewFrame() override;
    Buffer* CreateConstantBuffer(int32_t size) override;
    InternalBuffer* GetInternalBuffer() override { return nullptr; }
    int32_t GetOffset() override { return uniformBufferOffset_; }
    
    // ========== WebGPU-Specific ==========
    
    /**
     * Allocate from the uniform buffer ring
     */
    MemoryAllocationWebGPU Allocate(int32_t size);
    
    /**
     * Get the underlying uniform buffer
     */
    WGPUBuffer GetUniformBuffer() const { return uniformBuffer_; }
    
    /**
     * Flush staging data to GPU
     */
    void Flush();
};

} // namespace LLGI
