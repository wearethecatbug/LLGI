#pragma once

/**
 * LLGI PipelineStateWebGPU - WebGPU Pipeline State Implementation
 * 
 * Wraps WGPURenderPipeline and WGPUComputePipeline.
 * Uses per-stage shaders like Vulkan (separate vertex/pixel/compute shaders).
 */

#include "../LLGI.PipelineState.h"
#include "LLGI.BaseWebGPU.h"

#include <array>

namespace LLGI
{

class GraphicsWebGPU;
class ShaderWebGPU;
class RenderPassPipelineStateWebGPU;

/**
 * WebGPU Pipeline State
 * 
 * Configuration uses inherited public members from PipelineState:
 * - Culling, Topology, IsBlendEnabled, BlendSrcFunc, etc.
 * - VertexLayouts[], VertexLayoutCount
 */
class PipelineStateWebGPU : public PipelineState
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    
    WGPURenderPipeline renderPipeline_ = nullptr;
    WGPUComputePipeline computePipeline_ = nullptr;
    WGPUPipelineLayout pipelineLayout_ = nullptr;
    WGPUBindGroupLayout bindGroupLayout_ = nullptr;
    
    // Per-stage shaders (like Vulkan)
    std::array<ShaderWebGPU*, static_cast<int>(ShaderStageType::Max)> shaders_;

    bool CreateBindGroupLayout();
    bool CreatePipelineLayout();
    bool CreateRenderPipeline();
    bool CreateComputePipeline();

public:
    PipelineStateWebGPU(GraphicsWebGPU* graphics);
    ~PipelineStateWebGPU() override;
    
    bool Initialize();
    
    void SetShader(ShaderStageType stage, Shader* shader) override;
    
    bool Compile() override;
    
    // ========== WebGPU-Specific ==========
    
    WGPURenderPipeline GetPipeline() const { return renderPipeline_; }
    WGPUComputePipeline GetComputePipeline() const { return computePipeline_; }
    WGPUPipelineLayout GetLayout() const { return pipelineLayout_; }
    WGPUBindGroupLayout GetBindGroupLayout() const { return bindGroupLayout_; }
};

} // namespace LLGI
