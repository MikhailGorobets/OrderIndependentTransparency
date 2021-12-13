#include "Common.hlsli"


void VSBlitTexture(uint vertexID: SV_VertexID, out float4 position: SV_Position, out float4 color: TEXCOORD)
{


}

void PSBlitTexture(float4 position: SV_Position, float4 color: TEXCOORD)
{
   
}

[numthreads(8, 8, 1)]
void CSClearUnorderedAccessViewUintin(uint3 id: SV_DispatchThreadID)
{
    
}

[numthreads(1, 1, 1)]
void CSClearBufferCounter(uint3 id: SV_DispatchThreadID)
{
    
}
