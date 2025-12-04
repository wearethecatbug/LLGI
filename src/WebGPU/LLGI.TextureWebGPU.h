#pragma once

/**
 * LLGI TextureWebGPU - WebGPU Texture Implementation
 * 
 * Wraps WGPUTexture for 2D textures, render targets, and depth buffers.
 */

#include "../LLGI.Texture.h"
#include "../LLGI.Graphics.h"
#include "LLGI.BaseWebGPU.h"

namespace LLGI
{

class GraphicsWebGPU;

/**
 * WebGPU Texture
 * 
 * Provides LLGI texture interface for WebGPU:
 * - 2D textures for sampling
 * - Render target textures
 * - Depth/stencil textures
 * - External textures (from swapchain)
 */
class TextureWebGPU : public Texture
{
private:
    GraphicsWebGPU* graphics_ = nullptr;
    ReferenceObject* owner_ = nullptr;
    
    WGPUTexture texture_ = nullptr;
    WGPUTextureView view_ = nullptr;
    WGPUTextureFormat wgpuFormat_ = WGPUTextureFormat_RGBA8Unorm;
    
    Vec3I textureSize_ = {0, 0, 0};
    TextureParameter parameter_;
    
    // For CPU data uploads
    std::vector<uint8_t> stagingData_;
    void* lockedData_ = nullptr;
    
    bool isExternalResource_ = false;

public:
    TextureWebGPU(GraphicsWebGPU* graphics);
    ~TextureWebGPU() override;
    
    /**
     * Initialize texture with full parameters
     */
    bool Initialize(const TextureParameter& parameter);
    
    /**
     * Initialize from external WebGPU texture handle
     * Used for swapchain textures provided by platform
     */
    bool InitializeFromExternal(uint64_t textureHandle);
    
    /**
     * Initialize from external texture with view
     * For integration with external rendering systems
     */
    bool InitializeFromExternal(WGPUTexture texture, WGPUTextureView view, 
                                 WGPUTextureFormat format, const Vec2I& size);
    
    // ========== Texture Interface ==========
    
    void* Lock() override;
    void* Lock(int32_t mipmapLevel) override;
    void Unlock() override;
    
    bool GetData(std::vector<uint8_t>& data) override;
    
    void GenerateMipMaps() override;
    
    Vec2I GetSizeAs2D() const override;
    TextureFormatType GetFormat() const override;
    
    // ========== WebGPU-Specific ==========
    
    Vec3I GetSize() const { return textureSize_; }
    const TextureParameter& GetParameter() const { return parameter_; }
    
    WGPUTexture GetTexture() const { return texture_; }
    WGPUTextureView GetView() const { return view_; }
    WGPUTextureFormat GetWGPUFormat() const { return wgpuFormat_; }
    
    /**
     * Create a view for specific mip level or array layer
     */
    WGPUTextureView CreateView(int32_t mipLevel = 0, int32_t arrayLayer = 0) const;
    
    /**
     * Upload CPU data to GPU texture
     */
    void UploadData(const void* data, int32_t size, int32_t mipLevel = 0);
};

} // namespace LLGI
