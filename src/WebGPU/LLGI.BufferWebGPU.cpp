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
    // Use bitwise checks to handle combined usage flags (e.g., Index | MapWrite)
    WGPUBufferUsageFlags wgpuUsage = WGPUBufferUsage_CopyDst;  // Always allow CPU uploads
    
    auto usageInt = static_cast<uint32_t>(usage);
    
    // Track if this is a GPU-side buffer (Vertex, Index, Uniform, Storage)
    bool isGpuBuffer = false;
    
    if (usageInt & static_cast<uint32_t>(BufferUsageType::Vertex))
    {
        wgpuUsage |= WGPUBufferUsage_Vertex;
        isGpuBuffer = true;
    }
    if (usageInt & static_cast<uint32_t>(BufferUsageType::Index))
    {
        wgpuUsage |= WGPUBufferUsage_Index;
        isGpuBuffer = true;
    }
    if (usageInt & static_cast<uint32_t>(BufferUsageType::Constant))
    {
        wgpuUsage |= WGPUBufferUsage_Uniform;
        // Uniform buffers need 256-byte alignment in WebGPU
        actualSize_ = (actualSize_ + 255) & ~255;
        isGpuBuffer = true;
    }
    if (usageInt & static_cast<uint32_t>(BufferUsageType::ComputeRead))
    {
        wgpuUsage |= WGPUBufferUsage_Storage;
        isGpuBuffer = true;
    }
    if (usageInt & static_cast<uint32_t>(BufferUsageType::ComputeWrite))
    {
        wgpuUsage |= WGPUBufferUsage_Storage;
        isGpuBuffer = true;
    }
    if (usageInt & static_cast<uint32_t>(BufferUsageType::CopySrc))
    {
        wgpuUsage |= WGPUBufferUsage_CopySrc;
    }
    if (usageInt & static_cast<uint32_t>(BufferUsageType::CopyDst))
    {
        wgpuUsage |= WGPUBufferUsage_CopyDst;
    }
    
    // WebGPU restriction: MapWrite can only be combined with CopySrc
    // For GPU buffers (Vertex/Index/Uniform/Storage), skip MapWrite - use CopyDst instead
    if (usageInt & static_cast<uint32_t>(BufferUsageType::MapWrite))
    {
        if (!isGpuBuffer)
        {
            wgpuUsage |= WGPUBufferUsage_MapWrite;
            // MapWrite implies CopySrc, not CopyDst
            wgpuUsage &= ~WGPUBufferUsage_CopyDst;
            wgpuUsage |= WGPUBufferUsage_CopySrc;
        }
        // else: ignore MapWrite for GPU buffers, CopyDst is already set
    }
    if (usageInt & static_cast<uint32_t>(BufferUsageType::MapRead))
    {
        wgpuUsage |= WGPUBufferUsage_MapRead;
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
