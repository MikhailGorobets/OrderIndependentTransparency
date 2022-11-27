cmake_minimum_required (VERSION 3.6)

add_subdirectory(3rd-party/DiligentCore)
add_subdirectory(3rd-party/DiligentTools)
add_subdirectory(3rd-party/DiligentFX)
add_subdirectory(3rd-party/DiligentSamples)

project(OrderIndependentTransparency CXX)


set(INCLUDE
    include/OrderIndependentTransparencySample.hpp
)

set(SOURCE
    source/OrderIndependentTransparencySample.cpp
)

set(SHADERS
    assets/Shaders/Common.hlsli
    assets/Shaders/GeometryOpaque.hlsl
    assets/Shaders/GeometryResolve.hlsl
    assets/Shaders/GeometryTransparent.hlsl
    assets/Shaders/GraphicsPrimitives.hlsl
)

set(RENDER_STATES
    assets/RenderStates/GeometryOpaque.drsn
    assets/RenderStates/GeometryResolve.drsn
    assets/RenderStates/GeometryTransparent.drsn
    assets/RenderStates/GraphicsPrimitives.drsn
    assets/RenderStates/RenderStatesLibrary.drsn
)


add_sample_app("OrderIndependentTransparency" "DiligentCommunity" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${RENDER_STATES}")
target_link_libraries(OrderIndependentTransparency PRIVATE Diligent-RenderStateNotation)
target_include_directories(OrderIndependentTransparency PRIVATE "include")
set_property(TARGET OrderIndependentTransparency PROPERTY CXX_STANDARD 20)
