#include "Common.hlsli"

RWTexture2D<uint>            TextureHead;
RWStructuredBuffer<ListNode> BufferLinkedList;
RWStructuredBuffer<uint>     BufferLinkedListCounter;

void VSMain(uint VertexID: SV_VertexID, uint InstanceID: SV_InstanceID, out float4 Position: SV_Position, out float4 Color: TEXCOORD)
{
    float2 Positions[] = { float2(+0.0, +0.5), float2(+0.5, -0.5), float2(-0.5, -0.5) };
    float3 Colors[] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };
    float3 InstancePositionOffsets[] = { float3(0.0, 0.0, 0.3), float3(0.5, 0.0, 0.4), float3(-0.5, 0.0, 0.5), float3(0.0, 0.5, 0.6), float3(0.0, -0.5, 0.7) };
    
    Color = float4(Colors[VertexID], 0.5);
    Position = float4(float3(Positions[VertexID], 0.0) + InstancePositionOffsets[InstanceID], 1.0f);
}

[earlydepthstencil]
void PSMain(float4 Position: SV_Position, uint Coverage: SV_Coverage, float4 Color: TEXCOORD)
{
    uint NodeIdx = 0;
    InterlockedAdd(BufferLinkedListCounter[0], 1, NodeIdx);
    
    uint StructCount;
    uint StructStride;
    BufferLinkedList.GetDimensions(StructCount, StructStride);
    if (NodeIdx < StructCount)
    {
        uint PrevHead = TEXTURE_UINT_CLEAR;
        InterlockedExchange(TextureHead[uint2(Position.xy)], NodeIdx, PrevHead);

        ListNode Node;
        Node.Color = PackColor(Color);
        Node.Depth = asuint(Position.z);
        Node.Next = PrevHead;
        Node.Coverage = Coverage;
    
        BufferLinkedList[NodeIdx] = Node;
    }
}
