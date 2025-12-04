#pragma once

/**
 * LLGI ShaderWebGPU - WebGPU Shader Implementation
 * 
 * Wraps WGPUShaderModule for WGSL shader programs.
 */

#include "../LLGI.Shader.h"
#include "LLGI.BaseWebGPU.h"

#include <string>
#include <vector>

namespace LLGI
{

class GraphicsWebGPU;

/**
 * Shader stage data for WebGPU
 */
struct ShaderStageDataWebGPU
{
    ShaderStageType stage = ShaderStageType::Vertex;
    WGPUShaderModule module = nullptr;
    std::string entryPoint = "main";
};

/**
 * WebGPU Shader
 * 
 * Provides LLGI shader interface for WebGPU:
 * - WGSL shader source compilation
 * - Multiple shader stages (vertex, fragment, compute)
 */
class ShaderWebGPU : public Shader
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    
    // Shader modules per stage
    std::vector<ShaderStageDataWebGPU> stages_;
    
    // Original source for debugging
    std::vector<uint8_t> sourceBuffer_;

public:
    ShaderWebGPU(GraphicsWebGPU* graphics);
    ~ShaderWebGPU() override;
    
    /**
     * Initialize shader from WGSL source
     * 
     * @param data Array of shader stage data (WGSL source as string)
     * @param count Number of shader stages
     */
    bool Initialize(DataStructure* data, int32_t count);
    
    /**
     * Get shader module for specific stage
     */
    WGPUShaderModule GetShaderModule(ShaderStageType stage) const;
    
    /**
     * Get entry point name for specific stage
     */
    const std::string& GetEntryPoint(ShaderStageType stage) const;
    
    /**
     * Get all shader stages
     */
    const std::vector<ShaderStageDataWebGPU>& GetStages() const { return stages_; }
    
    /**
     * Check if shader has specific stage
     */
    bool HasStage(ShaderStageType stage) const;
};

} // namespace LLGI
