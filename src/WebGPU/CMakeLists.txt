# LLGI WebGPU Backend
#
# Build options:
# - BUILD_WEBGPU=ON: Enable WebGPU backend
# - WEBGPU_EMSCRIPTEN=ON: Build for Emscripten (browser)
# - WEBGPU_DAWN=ON: Build with Dawn (native)

set(LLGI_WEBGPU_SOURCES
    LLGI.BaseWebGPU.h
    LLGI.BaseWebGPU.cpp
    LLGI.GraphicsWebGPU.h
    LLGI.GraphicsWebGPU.cpp
    LLGI.BufferWebGPU.h
    LLGI.BufferWebGPU.cpp
    LLGI.TextureWebGPU.h
    LLGI.TextureWebGPU.cpp
    LLGI.ShaderWebGPU.h
    LLGI.ShaderWebGPU.cpp
    LLGI.CommandListWebGPU.h
    LLGI.CommandListWebGPU.cpp
    LLGI.PipelineStateWebGPU.h
    LLGI.PipelineStateWebGPU.cpp
    LLGI.RenderPassWebGPU.h
    LLGI.RenderPassWebGPU.cpp
    LLGI.SingleFrameMemoryPoolWebGPU.h
    LLGI.SingleFrameMemoryPoolWebGPU.cpp
)

# Create source list with full paths
set(LLGI_WEBGPU_SOURCES_FULL)
foreach(src ${LLGI_WEBGPU_SOURCES})
    list(APPEND LLGI_WEBGPU_SOURCES_FULL "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
endforeach()

# Export sources to parent scope
set(LLGI_WEBGPU_SOURCES_FULL ${LLGI_WEBGPU_SOURCES_FULL} PARENT_SCOPE)

# WebGPU include directories
if(EMSCRIPTEN)
    # Emscripten's WebGPU headers
    # No additional includes needed - they come with emdawnwebgpu port
else()
    # Dawn native
    if(DEFINED DAWN_INCLUDE_DIR)
        set(LLGI_WEBGPU_INCLUDES ${DAWN_INCLUDE_DIR} PARENT_SCOPE)
    else()
        message(WARNING "DAWN_INCLUDE_DIR not set for native WebGPU build")
    endif()
endif()

# Platform-specific definitions
if(EMSCRIPTEN)
    set(LLGI_WEBGPU_DEFINITIONS "__EMSCRIPTEN__" PARENT_SCOPE)
else()
    set(LLGI_WEBGPU_DEFINITIONS "LLGI_WEBGPU_DAWN" PARENT_SCOPE)
endif()
