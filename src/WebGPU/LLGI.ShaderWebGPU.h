#pragma once

/**
 * LLGI ShaderWebGPU - WebGPU Shader Implementation
 * 
 * Wraps WGPUShaderModule for WGSL shader programs.
 * Like Vulkan's ShaderVulkan, this stores a single shader module.
 * The EffekseerRendererLLGI layer creates separate Shader objects for VS, PS, CS.
 */

#include "../LLGI.Shader.h"
#include "LLGI.BaseWebGPU.h"

#include <string>
#include <vector>

namespace LLGI
{

class GraphicsWebGPU;

/**
 * WebGPU Shader
 * 
 * Provides LLGI shader interface for WebGPU:
 * - WGSL shader source compilation
 * - Single shader module (matches Vulkan pattern)
 * - Default entry point is "main"
 */
class ShaderWebGPU : public Shader
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    
    // Single shader module (like Vulkan)
    WGPUShaderModule shaderModule_ = nullptr;
    
    // Entry point name
    std::string entryPoint_ = "main";
    
    // Original source for debugging
    std::vector<uint8_t> sourceBuffer_;

public:
    ShaderWebGPU(GraphicsWebGPU* graphics);
    ~ShaderWebGPU() override;
    
    /**
     * Initialize shader from WGSL source
     * 
     * @param data Array of shader data (expects single WGSL source as text)
     * @param count Number of data elements (must be 1)
     */
    bool Initialize(DataStructure* data, int32_t count);
    
    /**
     * Get the shader module
     */
    WGPUShaderModule GetShaderModule() const { return shaderModule_; }
    
    /**
     * Get entry point name
     */
    const std::string& GetEntryPoint() const { return entryPoint_; }
};

} // namespace LLGI
