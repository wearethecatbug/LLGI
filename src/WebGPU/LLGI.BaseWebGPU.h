#pragma once

/**
 * LLGI WebGPU Backend
 * 
 * This backend provides WebGPU support for LLGI, enabling Effekseer to run on:
 * - Native platforms via Dawn (Windows, macOS, Linux)
 * - Web browsers via Emscripten + emdawnwebgpu
 * 
 * Build options:
 * - BUILD_WEBGPU=ON: Enable WebGPU backend
 * - For Emscripten: Use --use-port=emdawnwebgpu
 * - For Native: Link with Dawn
 */

#include "../LLGI.Base.h"

// WebGPU headers
// For native builds, include Dawn's webgpu.h
// For Emscripten builds, include emscripten's webgpu.h
#ifdef __EMSCRIPTEN__
#include <webgpu/webgpu.h>
// Note: webgpu_cpp.h provides C++ wrappers but we only use C API
// #include <webgpu/webgpu_cpp.h>
#else
// Dawn native or webgpu-native
#include <webgpu/webgpu.h>
// Note: webgpu_cpp.h provides C++ wrappers but we only use C API
// #include <webgpu/webgpu_cpp.h>
#endif

// Compatibility typedefs for different webgpu.h versions
// Older versions use WGPUBufferUsageFlags, newer ones use WGPUBufferUsage
#ifndef WGPUBufferUsageFlags
typedef WGPUBufferUsage WGPUBufferUsageFlags;
#endif
#ifndef WGPUTextureUsageFlags
typedef WGPUTextureUsage WGPUTextureUsageFlags;
#endif

// =============================================================================
// WebGPU API 2024+ Compatibility Layer
// =============================================================================
// The WebGPU API has evolved - newer versions (Dawn 2024+, wgpu-native) use:
// - WGPUStringView instead of const char* for labels/code
// - WGPUShaderSourceWGSL instead of WGPUShaderModuleWGSLDescriptor
// - WGPUTexelCopyTextureInfo instead of WGPUImageCopyTexture
// - WGPUTexelCopyBufferLayout instead of WGPUTextureDataLayout

#include <cstring>

// Helper to create WGPUStringView from C string
inline WGPUStringView LLGI_WGPUStringView(const char* str) {
    WGPUStringView sv;
    sv.data = str;
    sv.length = str ? strlen(str) : WGPU_STRLEN;
    return sv;
}

// Null string view for optional labels
inline WGPUStringView LLGI_WGPUStringViewNull() {
    WGPUStringView sv;
    sv.data = nullptr;
    sv.length = WGPU_STRLEN;
    return sv;
}

// Compatibility typedefs for texture copy structures (if needed)
#ifndef WGPUImageCopyTexture
typedef WGPUTexelCopyTextureInfo WGPUImageCopyTexture;
#endif
#ifndef WGPUTextureDataLayout
typedef WGPUTexelCopyBufferLayout WGPUTextureDataLayout;
#endif

// Shader source compatibility
#ifndef WGPUShaderModuleWGSLDescriptor
typedef WGPUShaderSourceWGSL WGPUShaderModuleWGSLDescriptor;
#define WGPUSType_ShaderModuleWGSLDescriptor WGPUSType_ShaderSourceWGSL
#endif

// Helper to convert bool to WGPUOptionalBool (new API)
inline WGPUOptionalBool LLGI_WGPUOptionalBool(bool value) {
    return value ? WGPUOptionalBool_True : WGPUOptionalBool_False;
}

namespace LLGI
{

// Forward declarations
class GraphicsWebGPU;
class CommandListWebGPU;
class BufferWebGPU;
class TextureWebGPU;
class ShaderWebGPU;
class PipelineStateWebGPU;
class RenderPassWebGPU;
class SingleFrameMemoryPoolWebGPU;

/**
 * Convert LLGI texture format to WebGPU texture format
 */
WGPUTextureFormat ConvertTextureFormat(TextureFormatType format);

/**
 * Convert WebGPU texture format to LLGI texture format
 */
TextureFormatType ConvertTextureFormatToLLGI(WGPUTextureFormat format);

/**
 * Get bytes per pixel for a texture format
 */
int32_t GetTextureFormatBytesPerPixel(TextureFormatType format);

/**
 * Convert LLGI blend factor to WebGPU blend factor
 */
WGPUBlendFactor ConvertBlendFactor(BlendFuncType func);

/**
 * Convert LLGI blend operation to WebGPU blend operation
 */
WGPUBlendOperation ConvertBlendOperation(BlendEquationType eq);

/**
 * Convert LLGI compare function to WebGPU compare function
 */
WGPUCompareFunction ConvertCompareFunction(DepthFuncType func);

/**
 * Convert LLGI topology to WebGPU primitive topology
 */
WGPUPrimitiveTopology ConvertTopology(TopologyType topology);

/**
 * Convert LLGI cull mode to WebGPU cull mode
 */
WGPUCullMode ConvertCullMode(CullingMode mode);

} // namespace LLGI
