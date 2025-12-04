/**
 * LLGI SingleFrameMemoryPoolWebGPU Implementation
 */

#include "LLGI.SingleFrameMemoryPoolWebGPU.h"
#include "LLGI.GraphicsWebGPU.h"
#include "LLGI.BufferWebGPU.h"

namespace LLGI
{

SingleFrameMemoryPoolWebGPU::SingleFrameMemoryPoolWebGPU(GraphicsWebGPU* graphics,
                                                           int32_t constantBufferPoolSize,
                                                           int32_t drawingCount)
    : graphics_(graphics)
    , swapCount_(graphics->GetSwapBufferCount())
{
    // Calculate total buffer size for all frames
    // Align pool size to uniform alignment
    int32_t alignedPoolSize = (constantBufferPoolSize + UNIFORM_ALIGNMENT - 1) & ~(UNIFORM_ALIGNMENT - 1);
    uniformBufferSize_ = alignedPoolSize * swapCount_;
    
    // Create uniform buffer
    uniformBuffer_ = graphics_->CreateWGPUBuffer(
        WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
        static_cast<uint64_t>(uniformBufferSize_),
        false
    );
    
    // Initialize staging data
    stagingData_.resize(uniformBufferSize_);
    
    // Initialize per-frame offsets
    frameOffsets_.resize(swapCount_);
    for (int32_t i = 0; i < swapCount_; ++i)
    {
        frameOffsets_[i] = i * alignedPoolSize;
    }
}

SingleFrameMemoryPoolWebGPU::~SingleFrameMemoryPoolWebGPU()
{
    if (uniformBuffer_ != nullptr)
    {
        wgpuBufferDestroy(uniformBuffer_);
        wgpuBufferRelease(uniformBuffer_);
        uniformBuffer_ = nullptr;
    }
}

void SingleFrameMemoryPoolWebGPU::NewFrame()
{
    // Flush any pending data from previous frame
    Flush();
    
    // Move to next frame in ring buffer
    currentFrame_ = (currentFrame_ + 1) % swapCount_;
    uniformBufferOffset_ = frameOffsets_[currentFrame_];
}

Buffer* SingleFrameMemoryPoolWebGPU::CreateConstantBuffer(int32_t size)
{
    auto* buffer = new BufferWebGPU(graphics_);
    
    if (!buffer->InitializeAsShortTime(this, size))
    {
        SafeRelease(buffer);
        return nullptr;
    }
    
    return buffer;
}

Buffer* SingleFrameMemoryPoolWebGPU::CreateBufferInternal(int32_t size)
{
    return CreateConstantBuffer(size);
}

Buffer* SingleFrameMemoryPoolWebGPU::ReinitializeBuffer(Buffer* cb, int32_t size)
{
    // WebGPU doesn't support resizing buffers, create a new one
    SafeRelease(cb);
    return CreateConstantBuffer(size);
}

MemoryAllocationWebGPU SingleFrameMemoryPoolWebGPU::Allocate(int32_t size)
{
    MemoryAllocationWebGPU result = {};
    
    // Align size to uniform alignment
    int32_t alignedSize = (size + UNIFORM_ALIGNMENT - 1) & ~(UNIFORM_ALIGNMENT - 1);
    
    // Check if we have space in current frame's portion
    int32_t frameEnd = (currentFrame_ + 1 < swapCount_) 
                       ? frameOffsets_[currentFrame_ + 1] 
                       : uniformBufferSize_;
    
    if (uniformBufferOffset_ + alignedSize > frameEnd)
    {
        // Out of memory for this frame
        return result;
    }
    
    result.buffer = uniformBuffer_;
    result.offset = uniformBufferOffset_;
    result.size = alignedSize;
    
    uniformBufferOffset_ += alignedSize;
    
    return result;
}

void SingleFrameMemoryPoolWebGPU::Flush()
{
    if (uniformBuffer_ == nullptr || stagingData_.empty())
    {
        return;
    }
    
    // Calculate range to flush
    int32_t frameStart = frameOffsets_[currentFrame_];
    int32_t dataSize = uniformBufferOffset_ - frameStart;
    
    if (dataSize > 0)
    {
        WGPUQueue queue = graphics_->GetQueue();
        wgpuQueueWriteBuffer(queue, 
                             uniformBuffer_, 
                             static_cast<uint64_t>(frameStart),
                             stagingData_.data() + frameStart, 
                             static_cast<size_t>(dataSize));
    }
}

} // namespace LLGI
