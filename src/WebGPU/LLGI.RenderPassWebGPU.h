#pragma once

/**
 * LLGI RenderPassWebGPU - WebGPU Render Pass Implementation
 */

#include "../LLGI.RenderPass.h"
#include "LLGI.BaseWebGPU.h"

#include <vector>

namespace LLGI
{

class GraphicsWebGPU;
class TextureWebGPU;

/**
 * WebGPU Render Pass
 * 
 * Manages render target attachments and provides WGPURenderPassDescriptor
 */
class RenderPassWebGPU : public RenderPass
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    
    std::vector<TextureWebGPU*> colorTargets_;
    std::vector<TextureWebGPU*> resolveTargets_;
    TextureWebGPU* depthTarget_ = nullptr;
    TextureWebGPU* resolveDepthTarget_ = nullptr;
    
    Vec2I size_ = {0, 0};
    
    // Cached descriptor
    WGPURenderPassDescriptor descriptor_ = {};
    std::vector<WGPURenderPassColorAttachment> colorAttachments_;
    WGPURenderPassDepthStencilAttachment depthAttachment_ = {};
    bool hasDepth_ = false;

public:
    RenderPassWebGPU(GraphicsWebGPU* graphics);
    ~RenderPassWebGPU() override;
    
    /**
     * Initialize render pass with color targets
     */
    bool Initialize(TextureWebGPU** colorTargets, int32_t colorTargetCount, TextureWebGPU* depthTarget);
    
    /**
     * Initialize render pass with MSAA resolve targets
     */
    bool InitializeWithMSAA(TextureWebGPU* colorTarget, TextureWebGPU* resolveTarget,
                             TextureWebGPU* depthTarget, TextureWebGPU* resolveDepthTarget);
    
    // ========== RenderPass Interface ==========
    
    Vec2I GetSize() const;
    bool GetIsColorCleared() const override;
    bool GetIsDepthCleared() const override;
    Color8 GetClearColor() const override;
    
    // ========== WebGPU-Specific ==========
    
    const WGPURenderPassDescriptor& GetDescriptor() const { return descriptor_; }
    bool HasDepthTarget() const { return hasDepth_; }
    
    void SetClearColor(const Color8& color);
    void SetClearDepth(float depth);
    void SetIsColorCleared(bool cleared);
    void SetIsDepthCleared(bool cleared);
    
private:
    void BuildDescriptor();
};

/**
 * Render Pass Pipeline State for WebGPU
 * 
 * Caches information about render target formats for pipeline creation
 */
class RenderPassPipelineStateWebGPU : public RenderPassPipelineState
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    RenderPassPipelineStateKey key_;
    
    std::vector<WGPUTextureFormat> colorFormats_;
    WGPUTextureFormat depthFormat_ = WGPUTextureFormat_Undefined;
    bool hasDepth_ = false;

public:
    RenderPassPipelineStateWebGPU(GraphicsWebGPU* graphics);
    ~RenderPassPipelineStateWebGPU() override;
    
    bool Initialize(const RenderPassPipelineStateKey& key);
    
    const std::vector<WGPUTextureFormat>& GetColorFormats() const { return colorFormats_; }
    WGPUTextureFormat GetDepthFormat() const { return depthFormat_; }
    bool HasDepthTexture() const { return hasDepth_; }
    
    RenderPassPipelineStateKey GetKey() const override { return key_; }
};

} // namespace LLGI
