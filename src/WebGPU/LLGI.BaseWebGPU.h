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
#include <webgpu/webgpu_cpp.h>
#else
// Dawn native
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>
#endif

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
