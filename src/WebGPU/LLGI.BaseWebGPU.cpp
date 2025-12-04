#include "LLGI.BaseWebGPU.h"

namespace LLGI
{

WGPUTextureFormat ConvertTextureFormat(TextureFormatType format)
{
    switch (format)
    {
    case TextureFormatType::R8G8B8A8_UNORM:
        return WGPUTextureFormat_RGBA8Unorm;
    case TextureFormatType::B8G8R8A8_UNORM:
        return WGPUTextureFormat_BGRA8Unorm;
    case TextureFormatType::R8_UNORM:
        return WGPUTextureFormat_R8Unorm;
    case TextureFormatType::R16G16_FLOAT:
        return WGPUTextureFormat_RG16Float;
    case TextureFormatType::R16G16B16A16_FLOAT:
        return WGPUTextureFormat_RGBA16Float;
    case TextureFormatType::R32G32B32A32_FLOAT:
        return WGPUTextureFormat_RGBA32Float;
    case TextureFormatType::R8G8B8A8_UNORM_SRGB:
        return WGPUTextureFormat_RGBA8UnormSrgb;
    case TextureFormatType::B8G8R8A8_UNORM_SRGB:
        return WGPUTextureFormat_BGRA8UnormSrgb;
    case TextureFormatType::D32:
        return WGPUTextureFormat_Depth32Float;
    case TextureFormatType::D24S8:
        return WGPUTextureFormat_Depth24PlusStencil8;
    case TextureFormatType::D32S8:
        return WGPUTextureFormat_Depth32FloatStencil8;
    default:
        return WGPUTextureFormat_RGBA8Unorm;
    }
}

TextureFormatType ConvertTextureFormatToLLGI(WGPUTextureFormat format)
{
    switch (format)
    {
    case WGPUTextureFormat_RGBA8Unorm:
        return TextureFormatType::R8G8B8A8_UNORM;
    case WGPUTextureFormat_BGRA8Unorm:
        return TextureFormatType::B8G8R8A8_UNORM;
    case WGPUTextureFormat_R8Unorm:
        return TextureFormatType::R8_UNORM;
    case WGPUTextureFormat_RG16Float:
        return TextureFormatType::R16G16_FLOAT;
    case WGPUTextureFormat_RGBA16Float:
        return TextureFormatType::R16G16B16A16_FLOAT;
    case WGPUTextureFormat_RGBA32Float:
        return TextureFormatType::R32G32B32A32_FLOAT;
    case WGPUTextureFormat_RGBA8UnormSrgb:
        return TextureFormatType::R8G8B8A8_UNORM_SRGB;
    case WGPUTextureFormat_BGRA8UnormSrgb:
        return TextureFormatType::B8G8R8A8_UNORM_SRGB;
    case WGPUTextureFormat_Depth32Float:
        return TextureFormatType::D32;
    case WGPUTextureFormat_Depth24PlusStencil8:
        return TextureFormatType::D24S8;
    case WGPUTextureFormat_Depth32FloatStencil8:
        return TextureFormatType::D32S8;
    default:
        return TextureFormatType::Unknown;
    }
}

int32_t GetTextureFormatBytesPerPixel(TextureFormatType format)
{
    switch (format)
    {
    case TextureFormatType::R8_UNORM:
        return 1;
    case TextureFormatType::R16G16_FLOAT:
        return 4;
    case TextureFormatType::R8G8B8A8_UNORM:
    case TextureFormatType::B8G8R8A8_UNORM:
    case TextureFormatType::R8G8B8A8_UNORM_SRGB:
    case TextureFormatType::B8G8R8A8_UNORM_SRGB:
    case TextureFormatType::D32:
        return 4;
    case TextureFormatType::D24S8:
        return 4;
    case TextureFormatType::R16G16B16A16_FLOAT:
    case TextureFormatType::D32S8:
        return 8;
    case TextureFormatType::R32G32B32A32_FLOAT:
        return 16;
    default:
        return 4;
    }
}

WGPUBlendFactor ConvertBlendFactor(BlendFuncType func)
{
    switch (func)
    {
    case BlendFuncType::Zero:
        return WGPUBlendFactor_Zero;
    case BlendFuncType::One:
        return WGPUBlendFactor_One;
    case BlendFuncType::SrcColor:
        return WGPUBlendFactor_Src;
    case BlendFuncType::OneMinusSrcColor:
        return WGPUBlendFactor_OneMinusSrc;
    case BlendFuncType::SrcAlpha:
        return WGPUBlendFactor_SrcAlpha;
    case BlendFuncType::OneMinusSrcAlpha:
        return WGPUBlendFactor_OneMinusSrcAlpha;
    case BlendFuncType::DstAlpha:
        return WGPUBlendFactor_DstAlpha;
    case BlendFuncType::OneMinusDstAlpha:
        return WGPUBlendFactor_OneMinusDstAlpha;
    case BlendFuncType::DstColor:
        return WGPUBlendFactor_Dst;
    case BlendFuncType::OneMinusDstColor:
        return WGPUBlendFactor_OneMinusDst;
    default:
        return WGPUBlendFactor_One;
    }
}

WGPUBlendOperation ConvertBlendOperation(BlendEquationType eq)
{
    switch (eq)
    {
    case BlendEquationType::Add:
        return WGPUBlendOperation_Add;
    case BlendEquationType::Sub:
        return WGPUBlendOperation_Subtract;
    case BlendEquationType::ReverseSub:
        return WGPUBlendOperation_ReverseSubtract;
    case BlendEquationType::Min:
        return WGPUBlendOperation_Min;
    case BlendEquationType::Max:
        return WGPUBlendOperation_Max;
    default:
        return WGPUBlendOperation_Add;
    }
}

WGPUCompareFunction ConvertCompareFunction(DepthFuncType func)
{
    switch (func)
    {
    case DepthFuncType::Never:
        return WGPUCompareFunction_Never;
    case DepthFuncType::Less:
        return WGPUCompareFunction_Less;
    case DepthFuncType::Equal:
        return WGPUCompareFunction_Equal;
    case DepthFuncType::LessEqual:
        return WGPUCompareFunction_LessEqual;
    case DepthFuncType::Greater:
        return WGPUCompareFunction_Greater;
    case DepthFuncType::NotEqual:
        return WGPUCompareFunction_NotEqual;
    case DepthFuncType::GreaterEqual:
        return WGPUCompareFunction_GreaterEqual;
    case DepthFuncType::Always:
        return WGPUCompareFunction_Always;
    default:
        return WGPUCompareFunction_Less;
    }
}

WGPUPrimitiveTopology ConvertTopology(TopologyType topology)
{
    switch (topology)
    {
    case TopologyType::Point:
        return WGPUPrimitiveTopology_PointList;
    case TopologyType::Line:
        return WGPUPrimitiveTopology_LineList;
    case TopologyType::Triangle:
        return WGPUPrimitiveTopology_TriangleList;
    default:
        return WGPUPrimitiveTopology_TriangleList;
    }
}

WGPUCullMode ConvertCullMode(CullingMode mode)
{
    switch (mode)
    {
    case CullingMode::Clockwise:
        return WGPUCullMode_Front;
    case CullingMode::CounterClockwise:
        return WGPUCullMode_Back;
    case CullingMode::DoubleSide:
        return WGPUCullMode_None;
    default:
        return WGPUCullMode_None;
    }
}

} // namespace LLGI
