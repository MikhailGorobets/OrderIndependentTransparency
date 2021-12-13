cmake_minimum_required (VERSION 3.6)

project(OrderIndependentTransparency CXX)

set(SOURCE
    src/RenderStateArchive.hpp
    src/OrderIndependentTransparencySample.cpp
)

set(INCLUDE
    src/RenderStateArchive.cpp
    src/OrderIndependentTransparencySample.hpp
)

set(SHADERS
    assets/Shaders/Common.hlsli
    assets/Shaders/GeometryOpaque.hlsl
    assets/Shaders/GeometryResolve.hlsl
    assets/Shaders/GeometryTransparent.hlsl
    assets/Shaders/GraphicsPrimitives.hlsl
)

set(ASSETS
    assets/RenderStates/GeometryOpaque.drsn
    assets/RenderStates/GeometryResolve.drsn
    assets/RenderStates/GeometryTransparent.drsn
    assets/RenderStates/GraphicsPrimitives.drsn
)

add_sample_app("OrderIndependentTransparency" "DiligentCommunity/Samples" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

target_link_libraries(OrderIndependentTransparency PRIVATE Diligent-RenderStateNotationParser)
