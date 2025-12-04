/**
 * LLGI PipelineStateWebGPU Implementation
 * 
 * Uses per-stage shaders like Vulkan.
 */

#include "LLGI.PipelineStateWebGPU.h"
#include "LLGI.GraphicsWebGPU.h"
#include "LLGI.ShaderWebGPU.h"
#include "LLGI.RenderPassPipelineStateWebGPU.h"

#include <cassert>

namespace LLGI
{

PipelineStateWebGPU::PipelineStateWebGPU(GraphicsWebGPU* graphics)
    : graphics_(graphics)
{
    shaders_.fill(nullptr);
}

PipelineStateWebGPU::~PipelineStateWebGPU()
{
    if (renderPipeline_ != nullptr)
    {
        wgpuRenderPipelineRelease(renderPipeline_);
        renderPipeline_ = nullptr;
    }
    
    if (computePipeline_ != nullptr)
    {
        wgpuComputePipelineRelease(computePipeline_);
        computePipeline_ = nullptr;
    }
    
    if (pipelineLayout_ != nullptr)
    {
        wgpuPipelineLayoutRelease(pipelineLayout_);
        pipelineLayout_ = nullptr;
    }
    
    if (bindGroupLayout_ != nullptr)
    {
        wgpuBindGroupLayoutRelease(bindGroupLayout_);
        bindGroupLayout_ = nullptr;
    }
    
    for (auto& shader : shaders_)
    {
        SafeRelease(shader);
    }
    
    SafeRelease(renderPassPipelineState_);
}

void PipelineStateWebGPU::SetShader(ShaderStageType stage, Shader* shader)
{
    int index = static_cast<int>(stage);
    if (index < 0 || index >= static_cast<int>(ShaderStageType::Max))
    {
        return;
    }
    
    SafeRelease(shaders_[index]);
    shaders_[index] = static_cast<ShaderWebGPU*>(shader);
    SafeAddRef(shaders_[index]);
}

void PipelineStateWebGPU::SetVertexLayout(const VertexLayoutElement* elements, int32_t elementCount)
{
    vertexLayoutElements_.clear();
    vertexLayoutElements_.reserve(elementCount);
    
    for (int32_t i = 0; i < elementCount; ++i)
    {
        vertexLayoutElements_.push_back(elements[i]);
    }
}

void PipelineStateWebGPU::SetTopologyType(TopologyType topologyType)
{
    topology_ = topologyType;
}

void PipelineStateWebGPU::SetCullingMode(CullingMode cullingMode)
{
    cullingMode_ = cullingMode;
}

void PipelineStateWebGPU::SetIsDepthWriteEnabled(bool isEnabled)
{
    isDepthWriteEnabled_ = isEnabled;
}

void PipelineStateWebGPU::SetDepthFuncType(DepthFuncType depthFuncType)
{
    depthFunc_ = depthFuncType;
}

void PipelineStateWebGPU::SetIsBlendEnabled(bool isEnabled)
{
    isBlendEnabled_ = isEnabled;
}

void PipelineStateWebGPU::SetBlendFunc(BlendFuncType src, BlendFuncType dst)
{
    SetBlendFuncRGB(src, dst);
    SetBlendFuncAlpha(src, dst);
}

void PipelineStateWebGPU::SetBlendFuncRGB(BlendFuncType src, BlendFuncType dst)
{
    blendSrcFuncRGB_ = src;
    blendDstFuncRGB_ = dst;
}

void PipelineStateWebGPU::SetBlendFuncAlpha(BlendFuncType src, BlendFuncType dst)
{
    blendSrcFuncAlpha_ = src;
    blendDstFuncAlpha_ = dst;
}

void PipelineStateWebGPU::SetBlendEquation(BlendEquationType blendEquation)
{
    SetBlendEquationRGB(blendEquation);
    SetBlendEquationAlpha(blendEquation);
}

void PipelineStateWebGPU::SetBlendEquationRGB(BlendEquationType blendEquation)
{
    blendEquationRGB_ = blendEquation;
}

void PipelineStateWebGPU::SetBlendEquationAlpha(BlendEquationType blendEquation)
{
    blendEquationAlpha_ = blendEquation;
}

void PipelineStateWebGPU::SetRenderPassPipelineState(RenderPassPipelineState* renderPassPipelineState)
{
    SafeRelease(renderPassPipelineState_);
    renderPassPipelineState_ = static_cast<RenderPassPipelineStateWebGPU*>(renderPassPipelineState);
    SafeAddRef(renderPassPipelineState_);
}

bool PipelineStateWebGPU::Compile()
{
    // Check if we have required shaders
    bool hasVertexShader = shaders_[static_cast<int>(ShaderStageType::Vertex)] != nullptr;
    bool hasPixelShader = shaders_[static_cast<int>(ShaderStageType::Pixel)] != nullptr;
    bool hasComputeShader = shaders_[static_cast<int>(ShaderStageType::Compute)] != nullptr;
    
    // Need either VS+PS for graphics, or CS for compute
    if (!hasComputeShader && (!hasVertexShader || !hasPixelShader))
    {
        return false;
    }
    
    if (!CreateBindGroupLayout())
    {
        return false;
    }
    
    if (!CreatePipelineLayout())
    {
        return false;
    }
    
    // Check if this is a compute shader
    if (hasComputeShader)
    {
        return CreateComputePipeline();
    }
    else
    {
        return CreateRenderPipeline();
    }
}

bool PipelineStateWebGPU::CreateBindGroupLayout()
{
    // Create a default bind group layout
    // In production, this should be derived from shader reflection
    
    std::vector<WGPUBindGroupLayoutEntry> entries;
    
    // Add entries for uniform buffers (binding 0-3)
    for (uint32_t i = 0; i < NumConstantBuffer; ++i)
    {
        WGPUBindGroupLayoutEntry entry = {};
        entry.binding = i;
        entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        entry.buffer.type = WGPUBufferBindingType_Uniform;
        entry.buffer.hasDynamicOffset = false;
        entry.buffer.minBindingSize = 0;
        entries.push_back(entry);
    }
    
    // Add entries for textures (binding 4+)
    for (uint32_t i = 0; i < 4; ++i)  // Support up to 4 textures
    {
        // Texture
        WGPUBindGroupLayoutEntry texEntry = {};
        texEntry.binding = NumConstantBuffer + i * 2;
        texEntry.visibility = WGPUShaderStage_Fragment;
        texEntry.texture.sampleType = WGPUTextureSampleType_Float;
        texEntry.texture.viewDimension = WGPUTextureViewDimension_2D;
        texEntry.texture.multisampled = false;
        entries.push_back(texEntry);
        
        // Sampler
        WGPUBindGroupLayoutEntry samplerEntry = {};
        samplerEntry.binding = NumConstantBuffer + i * 2 + 1;
        samplerEntry.visibility = WGPUShaderStage_Fragment;
        samplerEntry.sampler.type = WGPUSamplerBindingType_Filtering;
        entries.push_back(samplerEntry);
    }
    
    WGPUBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.label = nullptr;
    layoutDesc.entryCount = entries.size();
    layoutDesc.entries = entries.data();
    
    bindGroupLayout_ = wgpuDeviceCreateBindGroupLayout(graphics_->GetDevice(), &layoutDesc);
    
    return bindGroupLayout_ != nullptr;
}

bool PipelineStateWebGPU::CreatePipelineLayout()
{
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.label = nullptr;
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = &bindGroupLayout_;
    
    pipelineLayout_ = wgpuDeviceCreatePipelineLayout(graphics_->GetDevice(), &layoutDesc);
    
    return pipelineLayout_ != nullptr;
}

bool PipelineStateWebGPU::CreateRenderPipeline()
{
    auto vertexShader = shaders_[static_cast<int>(ShaderStageType::Vertex)];
    auto pixelShader = shaders_[static_cast<int>(ShaderStageType::Pixel)];
    
    if (vertexShader == nullptr || pixelShader == nullptr)
    {
        return false;
    }
    
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = nullptr;
    pipelineDesc.layout = pipelineLayout_;
    
    // Vertex stage - use vertex shader
    pipelineDesc.vertex.module = vertexShader->GetShaderModule();
    pipelineDesc.vertex.entryPoint = vertexShader->GetEntryPoint().c_str();
    
    // Vertex buffer layout
    std::vector<WGPUVertexAttribute> attributes;
    uint64_t offset = 0;
    
    for (size_t i = 0; i < vertexLayoutElements_.size(); ++i)
    {
        const auto& elem = vertexLayoutElements_[i];
        
        WGPUVertexAttribute attr = {};
        attr.shaderLocation = static_cast<uint32_t>(i);
        attr.offset = offset;
        
        switch (elem.Format)
        {
        case VertexLayoutFormat::R32G32B32A32_FLOAT:
            attr.format = WGPUVertexFormat_Float32x4;
            offset += 16;
            break;
        case VertexLayoutFormat::R32G32B32_FLOAT:
            attr.format = WGPUVertexFormat_Float32x3;
            offset += 12;
            break;
        case VertexLayoutFormat::R32G32_FLOAT:
            attr.format = WGPUVertexFormat_Float32x2;
            offset += 8;
            break;
        case VertexLayoutFormat::R32_FLOAT:
            attr.format = WGPUVertexFormat_Float32;
            offset += 4;
            break;
        case VertexLayoutFormat::R8G8B8A8_UNORM:
            attr.format = WGPUVertexFormat_Unorm8x4;
            offset += 4;
            break;
        case VertexLayoutFormat::R8G8B8A8_UINT:
            attr.format = WGPUVertexFormat_Uint8x4;
            offset += 4;
            break;
        default:
            attr.format = WGPUVertexFormat_Float32x4;
            offset += 16;
            break;
        }
        
        attributes.push_back(attr);
    }
    
    WGPUVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = offset;
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout.attributeCount = attributes.size();
    vertexBufferLayout.attributes = attributes.data();
    
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    
    // Primitive state
    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology = ConvertTopology(topology_);
    primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    primitiveState.cullMode = ConvertCullMode(cullingMode_);
    pipelineDesc.primitive = primitiveState;
    
    // Depth stencil state
    WGPUDepthStencilState depthStencilState = {};
    bool hasDepth = renderPassPipelineState_ != nullptr && 
                    renderPassPipelineState_->HasDepthTexture();
    
    if (hasDepth)
    {
        depthStencilState.format = WGPUTextureFormat_Depth32Float;
        depthStencilState.depthWriteEnabled = isDepthWriteEnabled_;
        depthStencilState.depthCompare = ConvertCompareFunction(depthFunc_);
        depthStencilState.stencilFront.compare = WGPUCompareFunction_Always;
        depthStencilState.stencilFront.failOp = WGPUStencilOperation_Keep;
        depthStencilState.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
        depthStencilState.stencilFront.passOp = WGPUStencilOperation_Keep;
        depthStencilState.stencilBack = depthStencilState.stencilFront;
        depthStencilState.stencilReadMask = 0xFF;
        depthStencilState.stencilWriteMask = 0xFF;
        depthStencilState.depthBias = 0;
        depthStencilState.depthBiasSlopeScale = 0.0f;
        depthStencilState.depthBiasClamp = 0.0f;
        pipelineDesc.depthStencil = &depthStencilState;
    }
    
    // Multisample state
    WGPUMultisampleState multisampleState = {};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;
    multisampleState.alphaToCoverageEnabled = false;
    pipelineDesc.multisample = multisampleState;
    
    // Fragment state - use pixel shader
    WGPUFragmentState fragmentState = {};
    fragmentState.module = pixelShader->GetShaderModule();
    fragmentState.entryPoint = pixelShader->GetEntryPoint().c_str();
    
    // Color target state
    WGPUColorTargetState colorTarget = {};
    colorTarget.format = WGPUTextureFormat_BGRA8Unorm;  // Default, should come from render pass
    colorTarget.writeMask = WGPUColorWriteMask_All;
    
    if (isBlendEnabled_)
    {
        WGPUBlendState blendState = {};
        blendState.color.srcFactor = ConvertBlendFactor(blendSrcFuncRGB_);
        blendState.color.dstFactor = ConvertBlendFactor(blendDstFuncRGB_);
        blendState.color.operation = ConvertBlendOperation(blendEquationRGB_);
        blendState.alpha.srcFactor = ConvertBlendFactor(blendSrcFuncAlpha_);
        blendState.alpha.dstFactor = ConvertBlendFactor(blendDstFuncAlpha_);
        blendState.alpha.operation = ConvertBlendOperation(blendEquationAlpha_);
        colorTarget.blend = &blendState;
    }
    
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
    
    renderPipeline_ = wgpuDeviceCreateRenderPipeline(graphics_->GetDevice(), &pipelineDesc);
    
    return renderPipeline_ != nullptr;
}

bool PipelineStateWebGPU::CreateComputePipeline()
{
    auto computeShader = shaders_[static_cast<int>(ShaderStageType::Compute)];
    
    if (computeShader == nullptr)
    {
        return false;
    }
    
    WGPUComputePipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = nullptr;
    pipelineDesc.layout = pipelineLayout_;
    pipelineDesc.compute.module = computeShader->GetShaderModule();
    pipelineDesc.compute.entryPoint = computeShader->GetEntryPoint().c_str();
    
    computePipeline_ = wgpuDeviceCreateComputePipeline(graphics_->GetDevice(), &pipelineDesc);
    
    return computePipeline_ != nullptr;
}

} // namespace LLGI
