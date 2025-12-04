/**
 * LLGI GraphicsWebGPU Implementation
 * 
 * WebGPU device wrapper providing LLGI interface for Effekseer
 */

#include "LLGI.GraphicsWebGPU.h"
#include "LLGI.BufferWebGPU.h"
#include "LLGI.CommandListWebGPU.h"
#include "LLGI.RenderPassWebGPU.h"  // Also contains RenderPassPipelineStateWebGPU
#include "LLGI.SingleFrameMemoryPoolWebGPU.h"
#include "LLGI.ShaderWebGPU.h"
#include "LLGI.PipelineStateWebGPU.h"
#include "LLGI.TextureWebGPU.h"

#include <cassert>
#include <cstring>

namespace LLGI
{

GraphicsWebGPU::GraphicsWebGPU(WGPUDevice device,
                               WGPUQueue queue,
                               int32_t swapBufferCount,
                               ReferenceObject* owner)
    : device_(device)
    , queue_(queue)
    , swapBufferCount_(swapBufferCount)
    , owner_(owner)
{
    SafeAddRef(owner_);
}

GraphicsWebGPU::~GraphicsWebGPU()
{
    renderPassPipelineStates_.clear();
    
    // Note: We don't destroy device/queue here as they're owned externally
    // (by the platform layer or browser)
    
    SafeRelease(owner_);
}

void GraphicsWebGPU::Execute(CommandList* commandList)
{
    auto* commandListWebGPU = static_cast<CommandListWebGPU*>(commandList);
    
    WGPUCommandBuffer cmdBuffer = commandListWebGPU->GetCommandBuffer();
    if (cmdBuffer != nullptr)
    {
        workCompleted_ = false;
        wgpuQueueSubmit(queue_, 1, &cmdBuffer);
    }
}

void GraphicsWebGPU::WaitFinish()
{
    // WebGPU doesn't have synchronous wait - we use queue onSubmittedWorkDone
    // For now, use a polling approach with device tick
    
    // In Dawn/native:
#if !defined(__EMSCRIPTEN__)
    // Dawn has wgpuDeviceTick to poll for completion
    // We'll implement proper async waiting in production
    
    // Placeholder: in production, use WGPUQueueWorkDoneCallback
    // For now, just tick the device to process pending work
    // wgpuDeviceTick(device_);
    
    // TODO: Implement proper async completion tracking
    // This requires maintaining a fence/semaphore mechanism
#else
    // In Emscripten, we typically don't block - the browser handles scheduling
    // Async operations complete on the next frame
#endif
    
    workCompleted_ = true;
}

Buffer* GraphicsWebGPU::CreateBuffer(BufferUsageType usage, int32_t size)
{
    auto buffer = new BufferWebGPU(this);
    if (!buffer->Initialize(usage, size))
    {
        SafeRelease(buffer);
        return nullptr;
    }
    return buffer;
}

Shader* GraphicsWebGPU::CreateShader(DataStructure* data, int32_t count)
{
    auto shader = new ShaderWebGPU(this);
    if (!shader->Initialize(data, count))
    {
        SafeRelease(shader);
        return nullptr;
    }
    return shader;
}

PipelineState* GraphicsWebGPU::CreatePiplineState()
{
    return new PipelineStateWebGPU(this);
}

SingleFrameMemoryPool* GraphicsWebGPU::CreateSingleFrameMemoryPool(int32_t constantBufferPoolSize, 
                                                                    int32_t drawingCount)
{
    auto pool = new SingleFrameMemoryPoolWebGPU(this, constantBufferPoolSize, drawingCount);
    return pool;
}

CommandList* GraphicsWebGPU::CreateCommandList(SingleFrameMemoryPool* memoryPool)
{
    auto commandList = new CommandListWebGPU(this);
    if (!commandList->Initialize(static_cast<SingleFrameMemoryPoolWebGPU*>(memoryPool)))
    {
        SafeRelease(commandList);
        return nullptr;
    }
    return commandList;
}

RenderPass* GraphicsWebGPU::CreateRenderPass(Texture** textures, 
                                              int32_t textureCount, 
                                              Texture* depthTexture)
{
    auto renderPass = new RenderPassWebGPU(this);
    
    std::vector<TextureWebGPU*> texturesWebGPU;
    texturesWebGPU.reserve(textureCount);
    
    for (int32_t i = 0; i < textureCount; ++i)
    {
        texturesWebGPU.push_back(static_cast<TextureWebGPU*>(textures[i]));
    }
    
    if (!renderPass->Initialize(texturesWebGPU.data(), 
                                 textureCount, 
                                 static_cast<TextureWebGPU*>(depthTexture)))
    {
        SafeRelease(renderPass);
        return nullptr;
    }
    
    return renderPass;
}

RenderPass* GraphicsWebGPU::CreateRenderPass(Texture* texture, 
                                              Texture* resolvedTexture,
                                              Texture* depthTexture, 
                                              Texture* resolvedDepthTexture)
{
    auto renderPass = new RenderPassWebGPU(this);
    
    if (!renderPass->InitializeWithMSAA(static_cast<TextureWebGPU*>(texture),
                                         static_cast<TextureWebGPU*>(resolvedTexture),
                                         static_cast<TextureWebGPU*>(depthTexture),
                                         static_cast<TextureWebGPU*>(resolvedDepthTexture)))
    {
        SafeRelease(renderPass);
        return nullptr;
    }
    
    return renderPass;
}

Texture* GraphicsWebGPU::CreateTexture(const TextureParameter& parameter)
{
    auto texture = new TextureWebGPU(this);
    if (!texture->Initialize(parameter))
    {
        SafeRelease(texture);
        return nullptr;
    }
    return texture;
}

Texture* GraphicsWebGPU::CreateTexture(uint64_t id)
{
    auto texture = new TextureWebGPU(this);
    if (!texture->InitializeFromExternal(id))
    {
        SafeRelease(texture);
        return nullptr;
    }
    return texture;
}

Texture* GraphicsWebGPU::CreateTexture(const TextureInitializationParameter& parameter)
{
    TextureParameter param;
    param.Format = parameter.Format;
    param.Size = Vec3I{parameter.Size.X, parameter.Size.Y, 1};
    param.Dimension = 2;
    param.MipLevelCount = parameter.MipMapCount > 0 ? parameter.MipMapCount : 1;
    param.SampleCount = 1;
    param.Usage = TextureUsageType::NoneFlag;
    
    return CreateTexture(param);
}

Texture* GraphicsWebGPU::CreateRenderTexture(const RenderTextureInitializationParameter& parameter)
{
    TextureParameter param;
    param.Format = parameter.Format;
    param.Size = Vec3I{parameter.Size.X, parameter.Size.Y, 1};
    param.Dimension = 2;
    param.MipLevelCount = 1;
    param.SampleCount = parameter.SamplingCount;
    param.Usage = TextureUsageType::RenderTarget;
    
    return CreateTexture(param);
}

Texture* GraphicsWebGPU::CreateDepthTexture(const DepthTextureInitializationParameter& parameter)
{
    TextureParameter param;
    param.Format = TextureFormatType::D32;  // Or D24S8 based on parameter
    param.Size = Vec3I{parameter.Size.X, parameter.Size.Y, 1};
    param.Dimension = 2;
    param.MipLevelCount = 1;
    param.SampleCount = parameter.SamplingCount;
    param.Usage = TextureUsageType::RenderTarget;  // Depth textures are render targets
    
    return CreateTexture(param);
}

RenderPassPipelineState* GraphicsWebGPU::CreateRenderPassPipelineState(RenderPass* renderpass)
{
    auto key = renderpass->GetKey();
    return CreateRenderPassPipelineState(key);
}

RenderPassPipelineState* GraphicsWebGPU::CreateRenderPassPipelineState(const RenderPassPipelineStateKey& key)
{
    // Check cache first
    auto it = renderPassPipelineStates_.find(key);
    if (it != renderPassPipelineStates_.end())
    {
        auto ret = it->second.get();
        SafeAddRef(ret);
        return ret;
    }
    
    // Create new pipeline state
    auto state = std::make_shared<RenderPassPipelineStateWebGPU>(this);
    if (!state->Initialize(key))
    {
        return nullptr;
    }
    
    renderPassPipelineStates_[key] = state;
    
    auto ret = state.get();
    SafeAddRef(ret);
    return ret;
}

std::vector<uint8_t> GraphicsWebGPU::CaptureRenderTarget(Texture* renderTarget)
{
    // TODO: Implement GPU readback
    // This requires:
    // 1. Create staging buffer with MapRead usage
    // 2. Copy texture to buffer
    // 3. Map buffer and read data
    // 4. Unmap and return
    
    auto* textureWebGPU = static_cast<TextureWebGPU*>(renderTarget);
    Vec2I size = textureWebGPU->GetSizeAs2D();
    
    std::vector<uint8_t> result;
    result.resize(size.X * size.Y * 4);  // RGBA8
    
    // Placeholder - actual implementation needs async buffer mapping
    return result;
}

Query* GraphicsWebGPU::CreateQuery(QueryType queryType, int32_t queryCount)
{
    // TODO: Implement query objects for timestamp/occlusion queries
    // WebGPU has WGPUQuerySet for this
    return nullptr;
}

uint64_t GraphicsWebGPU::TimestampToMicroseconds(uint64_t timestamp) const
{
    // WebGPU timestamps are in nanoseconds
    return timestamp / 1000;
}

WGPUBuffer GraphicsWebGPU::CreateWGPUBuffer(WGPUBufferUsageFlags usage, 
                                             uint64_t size, 
                                             bool mappedAtCreation)
{
    WGPUBufferDescriptor descriptor = {};
    descriptor.label = nullptr;
    descriptor.size = size;
    descriptor.usage = usage;
    descriptor.mappedAtCreation = mappedAtCreation;
    
    return wgpuDeviceCreateBuffer(device_, &descriptor);
}

WGPUTexture GraphicsWebGPU::CreateWGPUTexture(const WGPUTextureDescriptor& descriptor)
{
    return wgpuDeviceCreateTexture(device_, &descriptor);
}

WGPUSampler GraphicsWebGPU::CreateSampler(WGPUAddressMode addressMode, WGPUFilterMode filterMode)
{
    WGPUSamplerDescriptor descriptor = {};
    descriptor.label = nullptr;
    descriptor.addressModeU = addressMode;
    descriptor.addressModeV = addressMode;
    descriptor.addressModeW = addressMode;
    descriptor.magFilter = filterMode;
    descriptor.minFilter = filterMode;
    descriptor.mipmapFilter = (filterMode == WGPUFilterMode_Linear) 
                              ? WGPUMipmapFilterMode_Linear 
                              : WGPUMipmapFilterMode_Nearest;
    descriptor.lodMinClamp = 0.0f;
    descriptor.lodMaxClamp = 1000.0f;
    descriptor.maxAnisotropy = 1;
    
    return wgpuDeviceCreateSampler(device_, &descriptor);
}

} // namespace LLGI
