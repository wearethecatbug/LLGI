/**
 * LLGI ShaderWebGPU Implementation
 * 
 * Simplified to match Vulkan pattern - single shader module per Shader object.
 */

#include "LLGI.ShaderWebGPU.h"
#include "LLGI.GraphicsWebGPU.h"

#include <cassert>
#include <cstring>

namespace LLGI
{

ShaderWebGPU::ShaderWebGPU(GraphicsWebGPU* graphics)
    : graphics_(graphics)
{
}

ShaderWebGPU::~ShaderWebGPU()
{
    if (shaderModule_ != nullptr)
    {
        wgpuShaderModuleRelease(shaderModule_);
        shaderModule_ = nullptr;
    }
}

bool ShaderWebGPU::Initialize(DataStructure* data, int32_t count)
{
    // Like Vulkan, we expect exactly 1 data element
    if (count != 1)
    {
        return false;
    }
    
    if (data[0].Data == nullptr || data[0].Size <= 0)
    {
        return false;
    }
    
    WGPUDevice device = graphics_->GetDevice();
    
    // Store source for debugging
    sourceBuffer_.resize(data[0].Size);
    memcpy(sourceBuffer_.data(), data[0].Data, data[0].Size);
    
    // WGSL source is expected as null-terminated or sized string
    // Create a null-terminated copy
    std::string wgslSource(static_cast<const char*>(data[0].Data), data[0].Size);
    
    // Create shader module descriptor
    WGPUShaderModuleWGSLDescriptor wgslDesc = {};
    wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    wgslDesc.chain.next = nullptr;
    wgslDesc.code = wgslSource.c_str();
    
    WGPUShaderModuleDescriptor moduleDesc = {};
    moduleDesc.label = nullptr;
    moduleDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
    
    shaderModule_ = wgpuDeviceCreateShaderModule(device, &moduleDesc);
    if (shaderModule_ == nullptr)
    {
        // Shader compilation failed
        return false;
    }
    
    // Default entry point is "main"
    entryPoint_ = "main";
    
    return true;
}

} // namespace LLGI
