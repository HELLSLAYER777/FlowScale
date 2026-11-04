#include "gfx/CShader.h"
#include "util/Log.h"
#include <d3dcompiler.h>
#include <fstream>

#pragma comment(lib, "d3dcompiler.lib")

namespace fs {

bool CShader::Compile(ID3D11Device* dev, const std::wstring& file, const char* entry) {
    path_ = file;
    std::error_code ec;
    mtime_ = std::filesystem::last_write_time(file, ec);
    if (ec) { lastError_ = "cannot stat shader file"; return false; }

    std::ifstream f(file, std::ios::binary);
    if (!f) { lastError_ = "cannot open shader file"; return false; }
    std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    CPtr<ID3DBlob> code, errs;
    const HRESULT hr = D3DCompile(src.data(), src.size(), WideToUtf8(file).c_str(),
                                  /*defines*/ nullptr, /*includes*/ nullptr,
                                  entry, "cs_5_0",
                                  D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
                                  code.GetAddressOf(), errs.GetAddressOf());
    if (FAILED(hr)) {
        lastError_ = errs ? static_cast<const char*>(errs->GetBufferPointer()) : "D3DCompile failed";
        LOGE("Shader %ls: %s", file.c_str(), lastError_.c_str());
        return false;
    }

    CPtr<ID3D11ComputeShader> cs;
    ThrowIfFailed(dev->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(),
                                           nullptr, cs.GetAddressOf()), "CreateComputeShader");
    cs_ = std::move(cs);
    lastError_.clear();
    LOGI("Compiled shader %ls", file.c_str());
    return true;
}

int CShader::RecompileIfChanged(ID3D11Device* dev) {
    std::error_code ec;
    const auto mt = std::filesystem::last_write_time(path_, ec);
    if (ec) return -1;
    if (mt == mtime_ && cs_) return -1;
    // Preserve the working binary if the new source fails to compile.
    CPtr<ID3D11ComputeShader> prev = cs_;
    if (!Compile(dev, path_.c_str())) { cs_ = std::move(prev); return -2; }
    return 0;
}

void CShader::Dispatch(ID3D11DeviceContext* ctx,
                       ID3D11ShaderResourceView* const* srvs, UINT srvCount,
                       ID3D11UnorderedAccessView* uav, const ShaderParamsCB& params,
                       ID3D11Buffer* paramsCB) {
    // Upload params.
    D3D11_MAPPED_SUBRESOURCE m{};
    if (SUCCEEDED(ctx->Map(paramsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        memcpy(m.pData, &params, sizeof(params));
        ctx->Unmap(paramsCB, 0);
    }

    ctx->CSSetConstantBuffers(0, 1, &paramsCB);
    ctx->CSSetShaderResources(0, srvCount, srvs);
    ctx->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
    ctx->CSSetShader(cs_.Get(), nullptr, 0);

    const UINT gx = (params.dstSize[0] + 7) / 8;
    const UINT gy = (params.dstSize[1] + 7) / 8;
    ctx->Dispatch(gx, gy, 1);

    // Unbind so textures can be rebound as SRVs in the next pass without warnings.
    ID3D11UnorderedAccessView* nullUAV = nullptr;
    ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
    ID3D11ShaderResourceView* nullSRV[8] = {};
    ctx->CSSetShaderResources(0, srvCount, nullSRV);
    ctx->CSSetShader(nullptr, nullptr, 0);
}

} // namespace fs
