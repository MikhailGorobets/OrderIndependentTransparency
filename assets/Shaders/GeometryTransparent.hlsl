#include "Common.hlsli"

RWTexture2D<uint>            TextureHead;
RWStructuredBuffer<ListNode> BufferLinkedList;
RWStructuredBuffer<uint>     BufferLinkedListCounter;


void VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID, out float4 position : SV_Position, out float4 color : TEXCOORD) {

    float2 positions[] = { float2(+0.0, +0.5), float2(+0.5, -0.5), float2(-0.5, -0.5) };
    float3 colors[]    = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };
    float3 instancePositionOffsets[] = { float3(0.0, 0.0, 0.3), float3(0.5, 0.0, 0.4), float3(-0.5, 0.0, 0.5), float3(0.0, 0.5, 0.6), float3(0.0, -0.5, 0.7) };
    
    color    = float4(colors[vertexID], 0.5);
    position = float4(float3(positions[vertexID], 0.0) + instancePositionOffsets[instanceID], 1.0f);
}

[earlydepthstencil]
void PSMain(float4 position : SV_Position, uint coverage : SV_Coverage, float4 color : TEXCOORD) {
    uint nodeIdx = 0; 
    InterlockedAdd(BufferLinkedListCounter[0], 1, nodeIdx);
    
    uint numStructs;
    BufferLinkedList.GetDimensions(numStructs, 0);
    if (nodeIdx < numStructs)
    {
        uint prevHead = 0xFFFFFFFF;
        InterlockedExchange(TextureHead[uint2(position.xy)], nodeIdx, prevHead);

        ListNode node;
        node.Color = PackColor(color);
        node.Depth = asuint(position.z);
        node.Next = prevHead;
        node.Coverage = coverage;
    
        BufferLinkedList[nodeIdx] = node;
    }     
}
