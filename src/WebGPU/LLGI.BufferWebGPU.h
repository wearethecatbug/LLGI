#pragma once

/**
 * LLGI BufferWebGPU - WebGPU Buffer Implementation
 * 
 * Wraps WGPUBuffer for vertex, index, uniform, and storage buffer operations.
 */

#include "../LLGI.Buffer.h"
#include "LLGI.BaseWebGPU.h"

namespace LLGI
{

class GraphicsWebGPU;
class SingleFrameMemoryPoolWebGPU;

/**
 * WebGPU Buffer
 * 
 * Provides LLGI buffer interface for WebGPU:
 * - Vertex buffers (WGPUBufferUsage_Vertex)
 * - Index buffers (WGPUBufferUsage_Index)
 * - Uniform/constant buffers (WGPUBufferUsage_Uniform)
 * - Storage buffers (WGPUBufferUsage_Storage)
 */
class BufferWebGPU : public Buffer
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    WGPUBuffer buffer_ = nullptr;
    
    int32_t size_ = 0;
    int32_t actualSize_ = 0;
    int32_t offset_ = 0;
    
    // Staging buffer for CPU writes (WebGPU requires explicit staging)
    std::vector<uint8_t> stagingData_;
    bool isDirty_ = false;
    
    // For short-time buffers allocated from memory pool
    bool isFromPool_ = false;

public:
    BufferWebGPU(GraphicsWebGPU* graphics);
    ~BufferWebGPU() override;
    
    /**
     * Initialize buffer with specified usage and size
     */
    bool Initialize(BufferUsageType usage, int32_t size);
    
    /**
     * Initialize as short-time buffer from memory pool
     */
    bool InitializeAsShortTime(SingleFrameMemoryPoolWebGPU* memoryPool, int32_t size);
    
    // ========== Buffer Interface ==========
    
    /**
     * Lock buffer for CPU write access
     * Returns pointer to staging memory
     */
    void* Lock() override;
    
    /**
     * Lock specific region of buffer
     */
    void* Lock(int32_t offset, int32_t size) override;
    
    /**
     * Unlock buffer and upload staging data to GPU
     */
    void Unlock() override;
    
    /**
     * Get buffer size (may differ from actual GPU allocation)
     */
    int32_t GetSize() override;
    
    // ========== WebGPU-Specific ==========
    
    int32_t GetActualSize() const { return actualSize_; }
    int32_t GetOffset() const { return offset_; }
    
    WGPUBuffer GetBuffer() const { return buffer_; }
    
    /**
     * Flush staging data to GPU buffer
     * Called automatically on Unlock(), but can be called explicitly
     */
    void FlushToGPU();
};

} // namespace LLGI
