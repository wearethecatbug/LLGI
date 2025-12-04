/**
 * LLGI ShaderWebGPU Implementation
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
    for (auto& stage : stages_)
    {
        if (stage.module != nullptr)
        {
            wgpuShaderModuleRelease(stage.module);
            stage.module = nullptr;
        }
    }
    stages_.clear();
}

bool ShaderWebGPU::Initialize(DataStructure* data, int32_t count)
{
    if (data == nullptr || count <= 0)
    {
        return false;
    }
    
    WGPUDevice device = graphics_->GetDevice();
    
    for (int32_t i = 0; i < count; ++i)
    {
        const DataStructure& stageData = data[i];
        
        if (stageData.Data == nullptr || stageData.Size <= 0)
        {
            continue;
        }
        
        // WGSL source is expected as null-terminated string
        // Copy to ensure null termination
        std::string wgslSource(static_cast<const char*>(stageData.Data), stageData.Size);
        
        // Create shader module descriptor
        WGPUShaderModuleWGSLDescriptor wgslDesc = {};
        wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        wgslDesc.chain.next = nullptr;
        wgslDesc.code = wgslSource.c_str();
        
        WGPUShaderModuleDescriptor moduleDesc = {};
        moduleDesc.label = nullptr;
        moduleDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
        
        WGPUShaderModule module = wgpuDeviceCreateShaderModule(device, &moduleDesc);
        if (module == nullptr)
        {
            // Shader compilation failed
            // In production, should capture compilation info
            return false;
        }
        
        ShaderStageDataWebGPU stageInfo;
        stageInfo.stage = stageData.Stage;
        stageInfo.module = module;
        stageInfo.entryPoint = "main";  // Default entry point
        
        // Check for custom entry point in data
        if (stageData.EntryPoint != nullptr && stageData.EntryPoint[0] != '\0')
        {
            stageInfo.entryPoint = stageData.EntryPoint;
        }
        
        stages_.push_back(stageInfo);
    }
    
    return !stages_.empty();
}

WGPUShaderModule ShaderWebGPU::GetShaderModule(ShaderStageType stage) const
{
    for (const auto& s : stages_)
    {
        if (s.stage == stage)
        {
            return s.module;
        }
    }
    return nullptr;
}

const std::string& ShaderWebGPU::GetEntryPoint(ShaderStageType stage) const
{
    static const std::string empty;
    
    for (const auto& s : stages_)
    {
        if (s.stage == stage)
        {
            return s.entryPoint;
        }
    }
    return empty;
}

bool ShaderWebGPU::HasStage(ShaderStageType stage) const
{
    for (const auto& s : stages_)
    {
        if (s.stage == stage)
        {
            return true;
        }
    }
    return false;
}

} // namespace LLGI
