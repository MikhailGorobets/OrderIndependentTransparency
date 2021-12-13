#include "RenderDevice.h"
#include "RefCntAutoPtr.hpp"
#include <unordered_map>

namespace Diligent
{

class ShaderResourceBindingArchive final
{
public:
    ShaderResourceBindingArchive();

    void CreateShaderResourceBinding(const std::string& Name, IPipelineState* pPSO);

    void ReleaseShaderResourceBinding(const std::string& Name);
  
    IShaderResourceBinding* GetShaderResourceBinding(const std::string& Name);
 
private:
    std::unordered_map<std::string, RefCntAutoPtr<IShaderResourceBinding>> m_pShaderResourceBindings;
};

} // namespace Diligent
