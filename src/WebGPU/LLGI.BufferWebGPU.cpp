/**
 * LLGI BufferWebGPU Implementation
 */

#include "LLGI.BufferWebGPU.h"
#include "LLGI.GraphicsWebGPU.h"
#include "LLGI.SingleFrameMemoryPoolWebGPU.h"

#include <cassert>
#include <cstring>

namespace LLGI
{

BufferWebGPU::BufferWebGPU(GraphicsWebGPU* graphics)
    : graphics_(graphics)
{
}

BufferWebGPU::~BufferWebGPU()
{
    if (buffer_ != nullptr && !isFromPool_)
    {
        wgpuBufferDestroy(buffer_);
        wgpuBufferRelease(buffer_);
        buffer_ = nullptr;
    }
}

bool BufferWebGPU::Initialize(BufferUsageType usage, int32_t size)
{
    usage_ = usage;
    size_ = size;
    
    // Align size to 4 bytes (WebGPU requirement)
    actualSize_ = (size + 3) & ~3;
    
    // Determine WebGPU usage flags
    WGPUBufferUsageFlags wgpuUsage = WGPUBufferUsage_CopyDst;  // Always allow CPU uploads
    
    switch (usage)
    {
    case BufferUsageType::Vertex:
        wgpuUsage |= WGPUBufferUsage_Vertex;
        break;
    case BufferUsageType::Index:
        wgpuUsage |= WGPUBufferUsage_Index;
        break;
    case BufferUsageType::Constant:
        wgpuUsage |= WGPUBufferUsage_Uniform;
        // Uniform buffers need 256-byte alignment in WebGPU
        actualSize_ = (actualSize_ + 255) & ~255;
        break;
    case BufferUsageType::ComputeRead:
        wgpuUsage |= WGPUBufferUsage_Storage;
        break;
    case BufferUsageType::ComputeWrite:
        wgpuUsage |= WGPUBufferUsage_Storage;
        break;
    case BufferUsageType::CopySrc:
        wgpuUsage |= WGPUBufferUsage_CopySrc;
        break;
    case BufferUsageType::CopyDst:
        wgpuUsage |= WGPUBufferUsage_CopyDst;
        break;
    case BufferUsageType::MapWrite:
        wgpuUsage |= WGPUBufferUsage_MapWrite;
        break;
    case BufferUsageType::MapRead:
        wgpuUsage |= WGPUBufferUsage_MapRead;
        break;
    default:
        break;
    }
    
    buffer_ = graphics_->CreateWGPUBuffer(wgpuUsage, actualSize_, false);
    if (buffer_ == nullptr)
    {
        return false;
    }
    
    // Allocate staging buffer for CPU writes
    stagingData_.resize(actualSize_);
    
    return true;
}

bool BufferWebGPU::InitializeAsShortTime(SingleFrameMemoryPoolWebGPU* memoryPool, int32_t size)
{
    // Short-time buffers are allocated from a memory pool
    // They're valid only for the current frame
    
    isFromPool_ = true;
    size_ = size;
    actualSize_ = size;
    offset_ = 0;
    
    // Get buffer allocation from pool
    // The pool manages a large buffer and suballocates from it
    auto allocation = memoryPool->Allocate(size);
    if (allocation.buffer == nullptr)
    {
        return false;
    }
    
    buffer_ = allocation.buffer;
    offset_ = allocation.offset;
    actualSize_ = allocation.size;
    
    // For pool buffers, staging data points into pool's staging area
    stagingData_.resize(actualSize_);
    
    return true;
}

void* BufferWebGPU::Lock()
{
    return Lock(0, size_);
}

void* BufferWebGPU::Lock(int32_t offset, int32_t size)
{
    isDirty_ = true;
    return stagingData_.data() + offset;
}

void BufferWebGPU::Unlock()
{
    if (isDirty_)
    {
        FlushToGPU();
        isDirty_ = false;
    }
}

int32_t BufferWebGPU::GetSize()
{
    return size_;
}

void BufferWebGPU::FlushToGPU()
{
    if (buffer_ == nullptr || stagingData_.empty())
    {
        return;
    }
    
    WGPUQueue queue = graphics_->GetQueue();
    
    // Write staging data to GPU buffer
    wgpuQueueWriteBuffer(queue, 
                         buffer_, 
                         static_cast<uint64_t>(offset_),
                         stagingData_.data(), 
                         stagingData_.size());
}

} // namespace LLGI
