/**
 * LLGI RenderPassWebGPU Implementation
 */

#include "LLGI.RenderPassWebGPU.h"
#include "LLGI.GraphicsWebGPU.h"
#include "LLGI.TextureWebGPU.h"

namespace LLGI
{

// ============== RenderPassWebGPU ==============

RenderPassWebGPU::RenderPassWebGPU(GraphicsWebGPU* graphics)
    : graphics_(graphics)
{
}

RenderPassWebGPU::~RenderPassWebGPU()
{
    for (auto* target : colorTargets_)
    {
        SafeRelease(target);
    }
    colorTargets_.clear();
    
    for (auto* target : resolveTargets_)
    {
        SafeRelease(target);
    }
    resolveTargets_.clear();
    
    SafeRelease(depthTarget_);
    SafeRelease(resolveDepthTarget_);
}

bool RenderPassWebGPU::Initialize(TextureWebGPU** colorTargets, int32_t colorTargetCount, 
                                   TextureWebGPU* depthTarget)
{
    colorTargets_.clear();
    colorTargets_.reserve(colorTargetCount);
    
    for (int32_t i = 0; i < colorTargetCount; ++i)
    {
        SafeAddRef(colorTargets[i]);
        colorTargets_.push_back(colorTargets[i]);
    }
    
    if (depthTarget != nullptr)
    {
        SafeAddRef(depthTarget);
        depthTarget_ = depthTarget;
        hasDepth_ = true;
    }
    
    // Get size from first color target
    if (!colorTargets_.empty())
    {
        size_ = colorTargets_[0]->GetSizeAs2D();
    }
    else if (depthTarget_ != nullptr)
    {
        size_ = depthTarget_->GetSizeAs2D();
    }
    
    BuildDescriptor();
    
    return true;
}

bool RenderPassWebGPU::InitializeWithMSAA(TextureWebGPU* colorTarget, TextureWebGPU* resolveTarget,
                                           TextureWebGPU* depthTarget, TextureWebGPU* resolveDepthTarget)
{
    if (colorTarget != nullptr)
    {
        SafeAddRef(colorTarget);
        colorTargets_.push_back(colorTarget);
        
        if (resolveTarget != nullptr)
        {
            SafeAddRef(resolveTarget);
            resolveTargets_.push_back(resolveTarget);
        }
    }
    
    if (depthTarget != nullptr)
    {
        SafeAddRef(depthTarget);
        depthTarget_ = depthTarget;
        hasDepth_ = true;
        
        if (resolveDepthTarget != nullptr)
        {
            SafeAddRef(resolveDepthTarget);
            resolveDepthTarget_ = resolveDepthTarget;
        }
    }
    
    if (!colorTargets_.empty())
    {
        size_ = colorTargets_[0]->GetSizeAs2D();
    }
    else if (depthTarget_ != nullptr)
    {
        size_ = depthTarget_->GetSizeAs2D();
    }
    
    BuildDescriptor();
    
    return true;
}

Vec2I RenderPassWebGPU::GetSize() const
{
    return size_;
}

bool RenderPassWebGPU::GetIsColorCleared() const
{
    if (!colorAttachments_.empty())
    {
        return colorAttachments_[0].loadOp == WGPULoadOp_Clear;
    }
    return false;
}

bool RenderPassWebGPU::GetIsDepthCleared() const
{
    if (hasDepth_)
    {
        return depthAttachment_.depthLoadOp == WGPULoadOp_Clear;
    }
    return false;
}

Color8 RenderPassWebGPU::GetClearColor() const
{
    if (!colorAttachments_.empty())
    {
        const auto& c = colorAttachments_[0].clearValue;
        return Color8(
            static_cast<uint8_t>(c.r * 255),
            static_cast<uint8_t>(c.g * 255),
            static_cast<uint8_t>(c.b * 255),
            static_cast<uint8_t>(c.a * 255)
        );
    }
    return Color8(0, 0, 0, 255);
}

void RenderPassWebGPU::SetClearColor(const Color8& color)
{
    for (auto& attachment : colorAttachments_)
    {
        attachment.clearValue.r = color.R / 255.0;
        attachment.clearValue.g = color.G / 255.0;
        attachment.clearValue.b = color.B / 255.0;
        attachment.clearValue.a = color.A / 255.0;
    }
}

void RenderPassWebGPU::SetClearDepth(float depth)
{
    depthAttachment_.depthClearValue = depth;
}

void RenderPassWebGPU::SetIsColorCleared(bool cleared)
{
    WGPULoadOp op = cleared ? WGPULoadOp_Clear : WGPULoadOp_Load;
    for (auto& attachment : colorAttachments_)
    {
        attachment.loadOp = op;
    }
}

void RenderPassWebGPU::SetIsDepthCleared(bool cleared)
{
    depthAttachment_.depthLoadOp = cleared ? WGPULoadOp_Clear : WGPULoadOp_Load;
    depthAttachment_.stencilLoadOp = cleared ? WGPULoadOp_Clear : WGPULoadOp_Load;
}

void RenderPassWebGPU::BuildDescriptor()
{
    colorAttachments_.clear();
    colorAttachments_.reserve(colorTargets_.size());
    
    for (size_t i = 0; i < colorTargets_.size(); ++i)
    {
        WGPURenderPassColorAttachment attachment = {};
        attachment.view = colorTargets_[i]->GetView();
        attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        attachment.loadOp = WGPULoadOp_Clear;
        attachment.storeOp = WGPUStoreOp_Store;
        attachment.clearValue = {0.0, 0.0, 0.0, 1.0};
        
        // Handle MSAA resolve
        if (i < resolveTargets_.size() && resolveTargets_[i] != nullptr)
        {
            attachment.resolveTarget = resolveTargets_[i]->GetView();
        }
        
        colorAttachments_.push_back(attachment);
    }
    
    if (hasDepth_ && depthTarget_ != nullptr)
    {
        depthAttachment_ = {};
        depthAttachment_.view = depthTarget_->GetView();
        depthAttachment_.depthLoadOp = WGPULoadOp_Clear;
        depthAttachment_.depthStoreOp = WGPUStoreOp_Store;
        depthAttachment_.depthClearValue = 1.0f;
        depthAttachment_.stencilLoadOp = WGPULoadOp_Clear;
        depthAttachment_.stencilStoreOp = WGPUStoreOp_Store;
        depthAttachment_.stencilClearValue = 0;
        depthAttachment_.stencilReadOnly = false;
        depthAttachment_.depthReadOnly = false;
    }
    
    descriptor_ = {};
    descriptor_.label = nullptr;
    descriptor_.colorAttachmentCount = colorAttachments_.size();
    descriptor_.colorAttachments = colorAttachments_.data();
    descriptor_.depthStencilAttachment = hasDepth_ ? &depthAttachment_ : nullptr;
    descriptor_.timestampWrites = nullptr;
}

// ============== RenderPassPipelineStateWebGPU ==============

RenderPassPipelineStateWebGPU::RenderPassPipelineStateWebGPU(GraphicsWebGPU* graphics)
    : graphics_(graphics)
{
}

RenderPassPipelineStateWebGPU::~RenderPassPipelineStateWebGPU()
{
}

bool RenderPassPipelineStateWebGPU::Initialize(const RenderPassPipelineStateKey& key)
{
    key_ = key;
    
    // Convert LLGI formats to WebGPU formats
    colorFormats_.clear();
    
    for (int32_t i = 0; i < key.RenderTargetCount; ++i)
    {
        WGPUTextureFormat format = ConvertTextureFormat(key.RenderTargetFormats[i]);
        colorFormats_.push_back(format);
    }
    
    if (key.HasDepth)
    {
        hasDepth_ = true;
        depthFormat_ = ConvertTextureFormat(key.DepthFormat);
    }
    
    return true;
}

} // namespace LLGI
