/**
 * LLGI PipelineStateWebGPU Implementation
 * 
 * Uses per-stage shaders like Vulkan.
 * Configuration uses inherited public members from PipelineState base class.
 */

#include "LLGI.PipelineStateWebGPU.h"
#include "LLGI.GraphicsWebGPU.h"
#include "LLGI.ShaderWebGPU.h"
#include "LLGI.RenderPassWebGPU.h"  // Contains RenderPassPipelineStateWebGPU
#include "../LLGI.CommandList.h"    // For NumConstantBuffer

#include <cassert>
#include <vector>

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
}

bool PipelineStateWebGPU::Initialize()
{
    return true;
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
    
    // Vertex buffer layout - use base class VertexLayouts and VertexLayoutCount
    std::vector<WGPUVertexAttribute> attributes;
    uint64_t offset = 0;
    
    for (int i = 0; i < VertexLayoutCount; ++i)
    {
        WGPUVertexAttribute attr = {};
        attr.shaderLocation = static_cast<uint32_t>(i);
        attr.offset = offset;
        
        switch (VertexLayouts[i])
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
        case VertexLayoutFormat::R16G16_UNORM:
            attr.format = WGPUVertexFormat_Unorm16x2;
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
    
    if (VertexLayoutCount > 0)
    {
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;
    }
    else
    {
        pipelineDesc.vertex.bufferCount = 0;
        pipelineDesc.vertex.buffers = nullptr;
    }
    
    // Primitive state - use base class Topology and Culling
    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology = ConvertTopology(Topology);
    primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    primitiveState.cullMode = ConvertCullMode(Culling);
    pipelineDesc.primitive = primitiveState;
    
    // Depth stencil state - use base class IsDepthTestEnabled, IsDepthWriteEnabled, DepthFunc
    WGPUDepthStencilState depthStencilState = {};
    RenderPassPipelineStateWebGPU* rpps = nullptr;
    
    if (renderPassPipelineState_ != nullptr)
    {
        rpps = static_cast<RenderPassPipelineStateWebGPU*>(renderPassPipelineState_.get());
    }
    
    bool hasDepth = rpps != nullptr && rpps->HasDepthTexture();
    
    if (hasDepth || IsDepthTestEnabled)
    {
        depthStencilState.format = WGPUTextureFormat_Depth32Float;
        depthStencilState.depthWriteEnabled = IsDepthWriteEnabled;
        depthStencilState.depthCompare = ConvertCompareFunction(DepthFunc);
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
    
    // Color target state - use base class blend settings
    WGPUColorTargetState colorTarget = {};
    colorTarget.format = WGPUTextureFormat_BGRA8Unorm;  // Default, should come from render pass
    colorTarget.writeMask = WGPUColorWriteMask_All;
    
    WGPUBlendState blendState = {};
    if (IsBlendEnabled)
    {
        blendState.color.srcFactor = ConvertBlendFactor(BlendSrcFunc);
        blendState.color.dstFactor = ConvertBlendFactor(BlendDstFunc);
        blendState.color.operation = ConvertBlendOperation(BlendEquationRGB);
        blendState.alpha.srcFactor = ConvertBlendFactor(BlendSrcFuncAlpha);
        blendState.alpha.dstFactor = ConvertBlendFactor(BlendDstFuncAlpha);
        blendState.alpha.operation = ConvertBlendOperation(BlendEquationAlpha);
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
