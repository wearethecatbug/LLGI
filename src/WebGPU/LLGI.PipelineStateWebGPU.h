#pragma once

/**
 * LLGI PipelineStateWebGPU - WebGPU Pipeline State Implementation
 * 
 * Wraps WGPURenderPipeline and WGPUComputePipeline.
 */

#include "../LLGI.PipelineState.h"
#include "LLGI.BaseWebGPU.h"

#include <vector>

namespace LLGI
{

class GraphicsWebGPU;
class ShaderWebGPU;
class RenderPassPipelineStateWebGPU;

/**
 * WebGPU Pipeline State
 */
class PipelineStateWebGPU : public PipelineState
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    
    WGPURenderPipeline renderPipeline_ = nullptr;
    WGPUComputePipeline computePipeline_ = nullptr;
    WGPUPipelineLayout pipelineLayout_ = nullptr;
    WGPUBindGroupLayout bindGroupLayout_ = nullptr;
    
    ShaderWebGPU* shader_ = nullptr;
    RenderPassPipelineStateWebGPU* renderPassPipelineState_ = nullptr;
    
    // Pipeline configuration
    TopologyType topology_ = TopologyType::Triangle;
    CullingMode cullingMode_ = CullingMode::Clockwise;
    bool isDepthWriteEnabled_ = true;
    DepthFuncType depthFunc_ = DepthFuncType::Less;
    bool isBlendEnabled_ = false;
    BlendFuncType blendSrcFuncRGB_ = BlendFuncType::One;
    BlendFuncType blendDstFuncRGB_ = BlendFuncType::Zero;
    BlendFuncType blendSrcFuncAlpha_ = BlendFuncType::One;
    BlendFuncType blendDstFuncAlpha_ = BlendFuncType::Zero;
    BlendEquationType blendEquationRGB_ = BlendEquationType::Add;
    BlendEquationType blendEquationAlpha_ = BlendEquationType::Add;
    
    // Vertex layout
    std::vector<VertexLayoutElement> vertexLayoutElements_;

public:
    PipelineStateWebGPU(GraphicsWebGPU* graphics);
    ~PipelineStateWebGPU() override;
    
    // ========== PipelineState Interface ==========
    
    void SetShader(ShaderStageType stage, Shader* shader) override;
    void SetVertexLayout(const VertexLayoutElement* elements, int32_t elementCount) override;
    void SetTopologyType(TopologyType topologyType) override;
    void SetCullingMode(CullingMode cullingMode) override;
    void SetIsDepthWriteEnabled(bool isEnabled) override;
    void SetDepthFuncType(DepthFuncType depthFuncType) override;
    void SetIsBlendEnabled(bool isEnabled) override;
    void SetBlendFunc(BlendFuncType src, BlendFuncType dst) override;
    void SetBlendFuncRGB(BlendFuncType src, BlendFuncType dst) override;
    void SetBlendFuncAlpha(BlendFuncType src, BlendFuncType dst) override;
    void SetBlendEquation(BlendEquationType blendEquation) override;
    void SetBlendEquationRGB(BlendEquationType blendEquation) override;
    void SetBlendEquationAlpha(BlendEquationType blendEquation) override;
    void SetRenderPassPipelineState(RenderPassPipelineState* renderPassPipelineState) override;
    
    bool Compile() override;
    
    // ========== WebGPU-Specific ==========
    
    WGPURenderPipeline GetPipeline() const { return renderPipeline_; }
    WGPUComputePipeline GetComputePipeline() const { return computePipeline_; }
    WGPUPipelineLayout GetLayout() const { return pipelineLayout_; }
    WGPUBindGroupLayout GetBindGroupLayout() const { return bindGroupLayout_; }
    
    TopologyType GetTopology() const { return topology_; }
    
private:
    bool CreateBindGroupLayout();
    bool CreatePipelineLayout();
    bool CreateRenderPipeline();
    bool CreateComputePipeline();
};

} // namespace LLGI
