/**
 * LLGI CommandListWebGPU Implementation
 */

#include "LLGI.CommandListWebGPU.h"
#include "LLGI.GraphicsWebGPU.h"
#include "LLGI.SingleFrameMemoryPoolWebGPU.h"
#include "LLGI.RenderPassWebGPU.h"
#include "LLGI.PipelineStateWebGPU.h"
#include "LLGI.BufferWebGPU.h"
#include "LLGI.TextureWebGPU.h"

#include <cassert>
#include <cstring>

namespace LLGI
{

CommandListWebGPU::CommandListWebGPU(GraphicsWebGPU* graphics)
    : CommandList(graphics->GetSwapBufferCount())
    , graphics_(graphics)
{
}

CommandListWebGPU::~CommandListWebGPU()
{
    if (currentBindGroup_ != nullptr)
    {
        wgpuBindGroupRelease(currentBindGroup_);
        currentBindGroup_ = nullptr;
    }
    
    if (commandBuffer_ != nullptr)
    {
        wgpuCommandBufferRelease(commandBuffer_);
        commandBuffer_ = nullptr;
    }
    
    if (encoder_ != nullptr)
    {
        wgpuCommandEncoderRelease(encoder_);
        encoder_ = nullptr;
    }
}

bool CommandListWebGPU::Initialize(SingleFrameMemoryPoolWebGPU* memoryPool)
{
    memoryPool_ = memoryPool;
    return true;
}

void CommandListWebGPU::Begin()
{
    // Release previous command buffer if any
    if (commandBuffer_ != nullptr)
    {
        wgpuCommandBufferRelease(commandBuffer_);
        commandBuffer_ = nullptr;
    }
    
    if (encoder_ != nullptr)
    {
        wgpuCommandEncoderRelease(encoder_);
        encoder_ = nullptr;
    }
    
    // Create new command encoder
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label = LLGI_WGPUStringViewNull();
    
    encoder_ = wgpuDeviceCreateCommandEncoder(graphics_->GetDevice(), &encoderDesc);
    
    isInBegin_ = true;
    bindGroupDirty_ = true;
    
    CommandList::Begin();
}

bool CommandListWebGPU::BeginWithPlatform(void* platformContextPtr)
{
    if (platformContextPtr != nullptr)
    {
        // Use external command encoder provided by platform
        encoder_ = static_cast<WGPUCommandEncoder>(platformContextPtr);
        isExternalEncoder_ = true;
    }
    else
    {
        // Create our own encoder
        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = LLGI_WGPUStringViewNull();
        encoder_ = wgpuDeviceCreateCommandEncoder(graphics_->GetDevice(), &encoderDesc);
        isExternalEncoder_ = false;
    }
    
    isInBegin_ = true;
    bindGroupDirty_ = true;
    
    return encoder_ != nullptr;
}

void CommandListWebGPU::End()
{
    if (encoder_ == nullptr)
    {
        return;
    }
    
    // Only finish encoder if we created it (not external)
    if (!isExternalEncoder_)
    {
        // Finish encoding and create command buffer
        commandBuffer_ = wgpuCommandEncoderFinish(encoder_, nullptr);
        
        wgpuCommandEncoderRelease(encoder_);
    }
    
    encoder_ = nullptr;
    isExternalEncoder_ = false;
    isInBegin_ = false;
    
    CommandList::End();
}

void CommandListWebGPU::EndWithPlatform()
{
    // Don't finish the encoder - it's owned by platform
    if (isExternalEncoder_)
    {
        encoder_ = nullptr;
        isExternalEncoder_ = false;
        isInBegin_ = false;
    }
    else
    {
        End();
    }
}

void CommandListWebGPU::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    scissor_ = {x, y, width, height};
    hasScissor_ = true;
    
    if (renderPassEncoder_ != nullptr)
    {
        wgpuRenderPassEncoderSetScissorRect(renderPassEncoder_,
                                             static_cast<uint32_t>(x),
                                             static_cast<uint32_t>(y),
                                             static_cast<uint32_t>(width),
                                             static_cast<uint32_t>(height));
    }
}

void CommandListWebGPU::Draw(int32_t primitiveCount, int32_t instanceCount)
{
    if (renderPassEncoder_ == nullptr || currentPipeline_ == nullptr)
    {
        return;
    }
    
    FlushBindings();
    
    // Get topology from current pipeline to calculate vertex count
    TopologyType topology = currentPipeline_->GetTopology();
    int32_t vertexCount = 0;
    
    switch (topology)
    {
    case TopologyType::Triangle:
        vertexCount = primitiveCount * 3;
        break;
    case TopologyType::Line:
        vertexCount = primitiveCount * 2;
        break;
    case TopologyType::Point:
        vertexCount = primitiveCount;
        break;
    default:
        vertexCount = primitiveCount * 3;
        break;
    }
    
    // Check if we have index buffer bound
    BindingIndexBuffer indexBuffer;
    bool isDirtied;
    GetCurrentIndexBuffer(indexBuffer, isDirtied);
    
    if (indexBuffer.indexBuffer != nullptr)
    {
        // Indexed draw
        wgpuRenderPassEncoderDrawIndexed(renderPassEncoder_,
                                          static_cast<uint32_t>(vertexCount),
                                          static_cast<uint32_t>(instanceCount),
                                          0,  // firstIndex
                                          0,  // baseVertex
                                          0); // firstInstance
    }
    else
    {
        // Non-indexed draw
        wgpuRenderPassEncoderDraw(renderPassEncoder_,
                                   static_cast<uint32_t>(vertexCount),
                                   static_cast<uint32_t>(instanceCount),
                                   0,  // firstVertex
                                   0); // firstInstance
    }
}

void CommandListWebGPU::SetVertexBuffer(Buffer* vertexBuffer, int32_t stride, int32_t offset)
{
    CommandList::SetVertexBuffer(vertexBuffer, stride, offset);
    
    if (renderPassEncoder_ != nullptr && vertexBuffer != nullptr)
    {
        auto* bufferWebGPU = static_cast<BufferWebGPU*>(vertexBuffer);
        wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder_,
                                              0,  // slot
                                              bufferWebGPU->GetBuffer(),
                                              static_cast<uint64_t>(offset),
                                              static_cast<uint64_t>(bufferWebGPU->GetSize()));
    }
}

void CommandListWebGPU::SetIndexBuffer(Buffer* indexBuffer, int32_t stride, int32_t offset)
{
    CommandList::SetIndexBuffer(indexBuffer, stride, offset);
    
    if (renderPassEncoder_ != nullptr && indexBuffer != nullptr)
    {
        auto* bufferWebGPU = static_cast<BufferWebGPU*>(indexBuffer);
        
        WGPUIndexFormat format = (stride == 4) 
                                  ? WGPUIndexFormat_Uint32 
                                  : WGPUIndexFormat_Uint16;
        
        wgpuRenderPassEncoderSetIndexBuffer(renderPassEncoder_,
                                             bufferWebGPU->GetBuffer(),
                                             format,
                                             static_cast<uint64_t>(offset),
                                             static_cast<uint64_t>(bufferWebGPU->GetSize()));
    }
}

void CommandListWebGPU::SetPipelineState(PipelineState* pipelineState)
{
    CommandList::SetPipelineState(pipelineState);
    
    currentPipeline_ = static_cast<PipelineStateWebGPU*>(pipelineState);
    bindGroupDirty_ = true;
    
    if (renderPassEncoder_ != nullptr && currentPipeline_ != nullptr)
    {
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder_, 
                                          currentPipeline_->GetPipeline());
    }
}

void CommandListWebGPU::SetConstantBuffer(Buffer* constantBuffer, int32_t unit)
{
    CommandList::SetConstantBuffer(constantBuffer, unit);
    bindGroupDirty_ = true;
}

void CommandListWebGPU::SetTexture(Texture* texture, TextureWrapMode wrapMode, 
                                    TextureMinMagFilter minmagFilter, int32_t unit)
{
    CommandList::SetTexture(texture, wrapMode, minmagFilter, unit);
    bindGroupDirty_ = true;
}

void CommandListWebGPU::SetComputeBuffer(Buffer* computeBuffer, int32_t stride, 
                                          int32_t unit, bool is_readonly)
{
    CommandList::SetComputeBuffer(computeBuffer, stride, unit, is_readonly);
    bindGroupDirty_ = true;
}

void CommandListWebGPU::CopyTexture(Texture* src, Texture* dst)
{
    auto* srcTex = static_cast<TextureWebGPU*>(src);
    (void)static_cast<TextureWebGPU*>(dst);  // Validated by overload
    
    Vec2I srcSize = srcTex->GetSizeAs2D();
    
    CopyTexture(src, dst, {0, 0, 0}, {0, 0, 0}, {srcSize.X, srcSize.Y, 1}, 0, 0);
}

void CommandListWebGPU::CopyTexture(Texture* src, Texture* dst, 
                                     const Vec3I& srcPos, const Vec3I& dstPos,
                                     const Vec3I& size, int srcLayer, int dstLayer)
{
    if (encoder_ == nullptr)
    {
        return;
    }
    
    auto* srcTex = static_cast<TextureWebGPU*>(src);
    auto* dstTex = static_cast<TextureWebGPU*>(dst);
    
    WGPUImageCopyTexture srcCopy = {};
    srcCopy.texture = srcTex->GetTexture();
    srcCopy.mipLevel = 0;
    srcCopy.origin = {static_cast<uint32_t>(srcPos.X), 
                      static_cast<uint32_t>(srcPos.Y), 
                      static_cast<uint32_t>(srcLayer)};
    srcCopy.aspect = WGPUTextureAspect_All;
    
    WGPUImageCopyTexture dstCopy = {};
    dstCopy.texture = dstTex->GetTexture();
    dstCopy.mipLevel = 0;
    dstCopy.origin = {static_cast<uint32_t>(dstPos.X), 
                      static_cast<uint32_t>(dstPos.Y), 
                      static_cast<uint32_t>(dstLayer)};
    dstCopy.aspect = WGPUTextureAspect_All;
    
    WGPUExtent3D copySize = {};
    copySize.width = static_cast<uint32_t>(size.X);
    copySize.height = static_cast<uint32_t>(size.Y);
    copySize.depthOrArrayLayers = static_cast<uint32_t>(size.Z);
    
    wgpuCommandEncoderCopyTextureToTexture(encoder_, &srcCopy, &dstCopy, &copySize);
}

void CommandListWebGPU::GenerateMipMap(Texture* src)
{
    // WebGPU doesn't have automatic mipmap generation
    // Would need to implement via compute shader or blit chain
    // TODO: Implement mipmap generation
}

void CommandListWebGPU::BeginRenderPass(RenderPass* renderPass)
{
    if (encoder_ == nullptr || renderPass == nullptr)
    {
        return;
    }
    
    currentRenderPass_ = static_cast<RenderPassWebGPU*>(renderPass);
    
    // Get render pass descriptor from RenderPassWebGPU
    const WGPURenderPassDescriptor& desc = currentRenderPass_->GetDescriptor();
    
    renderPassEncoder_ = wgpuCommandEncoderBeginRenderPass(encoder_, &desc);
    
    isInRenderPass_ = true;
    
    // Set default viewport
    Vec2I size = currentRenderPass_->GetSize();
    viewport_ = {0, 0, static_cast<float>(size.X), static_cast<float>(size.Y), 0, 1};
    wgpuRenderPassEncoderSetViewport(renderPassEncoder_, 
                                      viewport_.x, viewport_.y, 
                                      viewport_.w, viewport_.h,
                                      viewport_.minDepth, viewport_.maxDepth);
    
    CommandList::BeginRenderPass(renderPass);
}

void CommandListWebGPU::EndRenderPass()
{
    if (renderPassEncoder_ != nullptr)
    {
        if (!isExternalRenderPass_)
        {
            wgpuRenderPassEncoderEnd(renderPassEncoder_);
            wgpuRenderPassEncoderRelease(renderPassEncoder_);
        }
        renderPassEncoder_ = nullptr;
    }
    
    currentRenderPass_ = nullptr;
    currentPipeline_ = nullptr;
    hasScissor_ = false;
    isExternalRenderPass_ = false;
    
    CommandList::EndRenderPass();
}

bool CommandListWebGPU::BeginRenderPassWithPlatformPtr(void* platformPtr)
{
    if (platformPtr != nullptr)
    {
        // Use external render pass encoder provided by platform
        renderPassEncoder_ = static_cast<WGPURenderPassEncoder>(platformPtr);
        isExternalRenderPass_ = true;
        isInRenderPass_ = true;
        return true;
    }
    return false;
}

bool CommandListWebGPU::EndRenderPassWithPlatformPtr()
{
    if (isExternalRenderPass_)
    {
        // Don't end or release - platform owns the encoder
        renderPassEncoder_ = nullptr;
        isExternalRenderPass_ = false;
        isInRenderPass_ = false;
        currentRenderPass_ = nullptr;
        currentPipeline_ = nullptr;
        hasScissor_ = false;
        return true;
    }
    return false;
}

void CommandListWebGPU::BeginComputePass()
{
    if (encoder_ == nullptr)
    {
        return;
    }
    
    WGPUComputePassDescriptor desc = {};
    desc.label = LLGI_WGPUStringViewNull();
    
    computePassEncoder_ = wgpuCommandEncoderBeginComputePass(encoder_, &desc);
}

void CommandListWebGPU::EndComputePass()
{
    if (computePassEncoder_ != nullptr)
    {
        wgpuComputePassEncoderEnd(computePassEncoder_);
        wgpuComputePassEncoderRelease(computePassEncoder_);
        computePassEncoder_ = nullptr;
    }
}

void CommandListWebGPU::Dispatch(int32_t groupX, int32_t groupY, int32_t groupZ,
                                  int32_t threadX, int32_t threadY, int32_t threadZ)
{
    if (computePassEncoder_ == nullptr)
    {
        return;
    }
    
    FlushBindings();
    
    wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder_,
                                              static_cast<uint32_t>(groupX),
                                              static_cast<uint32_t>(groupY),
                                              static_cast<uint32_t>(groupZ));
}

void CommandListWebGPU::CopyBuffer(Buffer* src, Buffer* dst)
{
    if (encoder_ == nullptr)
    {
        return;
    }
    
    auto* srcBuf = static_cast<BufferWebGPU*>(src);
    auto* dstBuf = static_cast<BufferWebGPU*>(dst);
    
    wgpuCommandEncoderCopyBufferToBuffer(encoder_,
                                          srcBuf->GetBuffer(),
                                          static_cast<uint64_t>(srcBuf->GetOffset()),
                                          dstBuf->GetBuffer(),
                                          static_cast<uint64_t>(dstBuf->GetOffset()),
                                          static_cast<uint64_t>(srcBuf->GetSize()));
}

void CommandListWebGPU::SetImageData2D(Texture* texture, int32_t x, int32_t y,
                                        int32_t width, int32_t height, const void* data)
{
    if (texture == nullptr || data == nullptr)
    {
        return;
    }
    
    auto* texWebGPU = static_cast<TextureWebGPU*>(texture);
    
    // Calculate data size
    int32_t bytesPerPixel = GetTextureFormatBytesPerPixel(texture->GetFormat());
    int32_t dataSize = width * height * bytesPerPixel;
    
    texWebGPU->UploadData(data, dataSize, 0);
}

void CommandListWebGPU::WaitUntilCompleted()
{
    graphics_->WaitFinish();
}

void CommandListWebGPU::CreateBindGroup()
{
    // TODO: Implement bind group creation based on current bindings
    // This requires:
    // 1. Get bind group layout from current pipeline
    // 2. Create entries for each bound resource (buffers, textures, samplers)
    // 3. Create bind group
    
    bindGroupDirty_ = false;
}

void CommandListWebGPU::FlushBindings()
{
    if (!bindGroupDirty_)
    {
        return;
    }
    
    CreateBindGroup();
    
    if (currentBindGroup_ != nullptr && renderPassEncoder_ != nullptr)
    {
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder_, 0, currentBindGroup_, 0, nullptr);
    }
}

} // namespace LLGI
