#include "Common.hlsli"

#define FRAGMENT_COUNT    8
#define MSAA_SAMPLE_COUNT 4

RWTexture2D<unorm float4>  TextureColor;
Texture2D<uint>            TextureHead;
StructuredBuffer<ListNode> BufferLinkedList;

[numthreads(8, 8, 1)]
void CSMain(uint3 Id: SV_DispatchThreadID)
{     
    float4 BackBuffer = TextureColor[Id.xy];
    float4 ResolveBuffer = float4(0.0, 0.0, 0.0, 0.0f);
    
    uint NodeHead = TextureHead[Id.xy];
    if (NodeHead == TEXTURE_UINT_CLEAR)
        return;
    
    ListSubNode Nodes[FRAGMENT_COUNT];
    for (uint SampleIdx = 0; SampleIdx < MSAA_SAMPLE_COUNT; SampleIdx++)
    {   
        uint Count = 0;
        uint NodeIdx = NodeHead;
     
        while (NodeIdx != TEXTURE_UINT_CLEAR && Count < FRAGMENT_COUNT)
        {
            ListNode Node = BufferLinkedList[NodeIdx];
            if (Node.Coverage & (1 << SampleIdx))
            {
                Nodes[Count].Depth = asfloat(Node.Depth);
                Nodes[Count].Color = Node.Color;
                Count++;
            }
            NodeIdx = Node.Next;
        }
              
        for (uint i = 1; i < Count; i++)
        {
            ListSubNode t = Nodes[i];
            uint j = i;
            while (j > 0 && (Nodes[j - 1].Depth < t.Depth))
            {
                Nodes[j] = Nodes[j - 1];
                j--;
            }
            Nodes[j] = t;
        }
         
        float4 DstPixelColor = BackBuffer;
        for (uint k = 0; k < Count; k++)
        {
            float4 SrcPixelColor = UnpackColor(Nodes[k].Color);
            DstPixelColor = lerp(DstPixelColor, SrcPixelColor, SrcPixelColor.a);
        }
        ResolveBuffer += DstPixelColor;
    }
    TextureColor[Id.xy] = ResolveBuffer / MSAA_SAMPLE_COUNT;
}
