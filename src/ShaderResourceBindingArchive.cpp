#include "ShaderResourceBindingArchive.hpp"

namespace Diligent
{

ShaderResourceBindingArchive::ShaderResourceBindingArchive() {}

void ShaderResourceBindingArchive::CreateShaderResourceBinding(const std::string& Name, IPipelineState* pPSO)
{
    VERIFY_EXPR(pPSO != nullptr);
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB);
    m_pShaderResourceBindings.emplace(Name, pSRB);
}

IShaderResourceBinding* ShaderResourceBindingArchive::GetShaderResourceBinding(const std::string& Name)
{
    auto Iter = m_pShaderResourceBindings.find(Name);
    if (Iter == m_pShaderResourceBindings.end())
        DEV_ERROR("Unable to find shader resource binding '", Name, "'.");
    return Iter->second;
}

void ShaderResourceBindingArchive::ReleaseShaderResourceBinding(const std::string& Name)
{
    auto Iter = m_pShaderResourceBindings.find(Name);
    if (Iter == m_pShaderResourceBindings.end())
        DEV_ERROR("Unable to find shader resource binding '", Name, "'.");
    m_pShaderResourceBindings.erase(Iter);
}

} // namespace Diligent
