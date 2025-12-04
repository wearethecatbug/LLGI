/**
 * LLGI TextureWebGPU Implementation
 */

#include "LLGI.TextureWebGPU.h"
#include "LLGI.GraphicsWebGPU.h"

#include <cassert>
#include <cstring>

namespace LLGI
{

TextureWebGPU::TextureWebGPU(GraphicsWebGPU* graphics)
    : graphics_(graphics)
{
}

TextureWebGPU::~TextureWebGPU()
{
    if (!isExternalResource_)
    {
        if (view_ != nullptr)
        {
            wgpuTextureViewRelease(view_);
            view_ = nullptr;
        }
        if (texture_ != nullptr)
        {
            wgpuTextureDestroy(texture_);
            wgpuTextureRelease(texture_);
            texture_ = nullptr;
        }
    }
    
    SafeRelease(owner_);
}

bool TextureWebGPU::Initialize(const TextureParameter& parameter)
{
    parameter_ = parameter;
    format_ = parameter.Format;
    usage_ = parameter.Usage;
    samplingCount_ = parameter.SampleCount;
    mipmapCount_ = parameter.MipLevelCount;
    
    // Determine texture dimensions
    textureSize_.X = parameter.Size.X;
    textureSize_.Y = parameter.Size.Y;
    textureSize_.Z = parameter.Dimension == 3 ? parameter.Size.Z : 1;
    
    // Convert format
    wgpuFormat_ = ConvertTextureFormat(parameter.Format);
    
    // Determine usage flags
    WGPUTextureUsageFlags wgpuUsage = WGPUTextureUsage_CopyDst;  // Allow uploads
    
    if ((static_cast<int>(parameter.Usage) & static_cast<int>(TextureUsageType::RenderTarget)) != 0)
    {
        wgpuUsage |= WGPUTextureUsage_RenderAttachment;
        type_ = TextureType::Render;
    }
    else
    {
        wgpuUsage |= WGPUTextureUsage_TextureBinding;  // Sampling
        type_ = TextureType::Color;
    }
    
    // Check for depth format
    if (parameter.Format == TextureFormatType::D32 ||
        parameter.Format == TextureFormatType::D24S8 ||
        parameter.Format == TextureFormatType::D32S8)
    {
        type_ = TextureType::Depth;
    }
    
    // Create texture descriptor
    WGPUTextureDescriptor descriptor = {};
    descriptor.label = nullptr;
    descriptor.size.width = static_cast<uint32_t>(textureSize_.X);
    descriptor.size.height = static_cast<uint32_t>(textureSize_.Y);
    descriptor.size.depthOrArrayLayers = static_cast<uint32_t>(textureSize_.Z);
    descriptor.mipLevelCount = static_cast<uint32_t>(mipmapCount_);
    descriptor.sampleCount = static_cast<uint32_t>(samplingCount_);
    descriptor.dimension = (parameter.Dimension == 3) 
                           ? WGPUTextureDimension_3D 
                           : WGPUTextureDimension_2D;
    descriptor.format = wgpuFormat_;
    descriptor.usage = wgpuUsage;
    
    texture_ = graphics_->CreateWGPUTexture(descriptor);
    if (texture_ == nullptr)
    {
        return false;
    }
    
    // Create default view
    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.label = nullptr;
    viewDesc.format = wgpuFormat_;
    viewDesc.dimension = (parameter.Dimension == 3) 
                         ? WGPUTextureViewDimension_3D 
                         : WGPUTextureViewDimension_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = static_cast<uint32_t>(mipmapCount_);
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;
    viewDesc.aspect = (type_ == TextureType::Depth) 
                      ? WGPUTextureAspect_DepthOnly 
                      : WGPUTextureAspect_All;
    
    view_ = wgpuTextureCreateView(texture_, &viewDesc);
    if (view_ == nullptr)
    {
        wgpuTextureDestroy(texture_);
        wgpuTextureRelease(texture_);
        texture_ = nullptr;
        return false;
    }
    
    return true;
}

bool TextureWebGPU::InitializeFromExternal(uint64_t textureHandle)
{
    // Cast handle back to WGPUTexture
    texture_ = reinterpret_cast<WGPUTexture>(textureHandle);
    isExternalResource_ = true;
    
    // For external textures, we need format/size info from caller
    // This is a simplified version - full implementation would query the texture
    type_ = TextureType::Screen;
    
    return texture_ != nullptr;
}

bool TextureWebGPU::InitializeFromExternal(WGPUTexture texture, WGPUTextureView view,
                                            WGPUTextureFormat format, const Vec2I& size)
{
    texture_ = texture;
    view_ = view;
    wgpuFormat_ = format;
    textureSize_ = {size.X, size.Y, 1};
    isExternalResource_ = true;
    type_ = TextureType::Screen;
    
    // Convert WebGPU format back to LLGI format
    format_ = ConvertTextureFormatToLLGI(format);
    
    return texture_ != nullptr;
}

void* TextureWebGPU::Lock()
{
    return Lock(0);
}

void* TextureWebGPU::Lock(int32_t mipmapLevel)
{
    // Calculate size for this mip level
    int32_t mipWidth = std::max(1, textureSize_.X >> mipmapLevel);
    int32_t mipHeight = std::max(1, textureSize_.Y >> mipmapLevel);
    int32_t bytesPerPixel = GetTextureFormatBytesPerPixel(format_);
    
    int32_t dataSize = mipWidth * mipHeight * bytesPerPixel;
    stagingData_.resize(dataSize);
    lockedData_ = stagingData_.data();
    
    return lockedData_;
}

void TextureWebGPU::Unlock()
{
    if (lockedData_ != nullptr && !stagingData_.empty())
    {
        UploadData(stagingData_.data(), static_cast<int32_t>(stagingData_.size()), 0);
        lockedData_ = nullptr;
    }
}

bool TextureWebGPU::GetData(std::vector<uint8_t>& data)
{
    // TODO: Implement GPU readback
    // Requires:
    // 1. Create staging buffer with MapRead
    // 2. Copy texture to buffer
    // 3. Map and read buffer (async in WebGPU)
    return false;
}

void TextureWebGPU::GenerateMipMaps()
{
    // WebGPU doesn't have automatic mipmap generation
    // Would need to implement via compute shader or blit operations
    // TODO: Implement mipmap generation
}

Vec2I TextureWebGPU::GetSizeAs2D() const
{
    return {textureSize_.X, textureSize_.Y};
}

TextureFormatType TextureWebGPU::GetFormat() const
{
    return format_;
}

WGPUTextureView TextureWebGPU::CreateView(int32_t mipLevel, int32_t arrayLayer) const
{
    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.label = nullptr;
    viewDesc.format = wgpuFormat_;
    viewDesc.dimension = WGPUTextureViewDimension_2D;
    viewDesc.baseMipLevel = static_cast<uint32_t>(mipLevel);
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = static_cast<uint32_t>(arrayLayer);
    viewDesc.arrayLayerCount = 1;
    viewDesc.aspect = (type_ == TextureType::Depth) 
                      ? WGPUTextureAspect_DepthOnly 
                      : WGPUTextureAspect_All;
    
    return wgpuTextureCreateView(texture_, &viewDesc);
}

void TextureWebGPU::UploadData(const void* data, int32_t size, int32_t mipLevel)
{
    if (texture_ == nullptr || data == nullptr || size <= 0)
    {
        return;
    }
    
    int32_t mipWidth = std::max(1, textureSize_.X >> mipLevel);
    int32_t mipHeight = std::max(1, textureSize_.Y >> mipLevel);
    int32_t bytesPerPixel = GetTextureFormatBytesPerPixel(format_);
    int32_t bytesPerRow = mipWidth * bytesPerPixel;
    
    // WebGPU requires bytesPerRow to be aligned to 256 bytes
    int32_t alignedBytesPerRow = (bytesPerRow + 255) & ~255;
    
    WGPUQueue queue = graphics_->GetQueue();
    
    WGPUImageCopyTexture destination = {};
    destination.texture = texture_;
    destination.mipLevel = static_cast<uint32_t>(mipLevel);
    destination.origin = {0, 0, 0};
    destination.aspect = WGPUTextureAspect_All;
    
    WGPUTextureDataLayout dataLayout = {};
    dataLayout.offset = 0;
    dataLayout.bytesPerRow = static_cast<uint32_t>(alignedBytesPerRow);
    dataLayout.rowsPerImage = static_cast<uint32_t>(mipHeight);
    
    WGPUExtent3D writeSize = {};
    writeSize.width = static_cast<uint32_t>(mipWidth);
    writeSize.height = static_cast<uint32_t>(mipHeight);
    writeSize.depthOrArrayLayers = 1;
    
    // If data is not aligned, we need to copy to aligned buffer
    if (bytesPerRow != alignedBytesPerRow)
    {
        std::vector<uint8_t> alignedData(alignedBytesPerRow * mipHeight);
        const uint8_t* srcRow = static_cast<const uint8_t*>(data);
        uint8_t* dstRow = alignedData.data();
        
        for (int32_t y = 0; y < mipHeight; ++y)
        {
            std::memcpy(dstRow, srcRow, bytesPerRow);
            srcRow += bytesPerRow;
            dstRow += alignedBytesPerRow;
        }
        
        wgpuQueueWriteTexture(queue, &destination, alignedData.data(), 
                              alignedData.size(), &dataLayout, &writeSize);
    }
    else
    {
        wgpuQueueWriteTexture(queue, &destination, data, 
                              static_cast<size_t>(size), &dataLayout, &writeSize);
    }
}

} // namespace LLGI
