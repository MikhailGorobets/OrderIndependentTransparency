#include "RenderDevice.h"
#include "RefCntAutoPtr.hpp"
#include <unordered_map>

namespace Diligent
{
class RenderStateArchive final
{
public:
    RenderStateArchive(RefCntAutoPtr<IRenderDevice>                   pRenderDevice,
                       RefCntAutoPtr<IShaderSourceInputStreamFactory> pStreamFactory,
                       const std::vector<std::string>&                DRSNFiles);

    IPipelineState* GetPipelineState(const std::string& Name);

private:
    std::unordered_map<std::string, RefCntAutoPtr<IPipelineState>> m_pPipelines;
};
} // namespace Diligent
