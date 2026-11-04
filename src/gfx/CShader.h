// FlowScale — compute-shader compilation + dispatch helpers.
//
// All HLSL in FlowScale is compiled at runtime with D3DCompile from the .hlsl
// files deployed next to the executable. That keeps a shader tweak a pure
// asset edit (F5 in the UI hot-reloads user shaders without a rebuild).
#pragma once
#include <filesystem>
#include "util/Common.h"

namespace fs {

#pragma pack(push, 4)
struct ShaderParamsCB {
    uint32_t srcSize[2];
    uint32_t dstSize[2];
    float    factor;
    float    sharpen;
    float    pad[2];
};
#pragma pack(pop)

class CShader {
public:
    // Compiles `file` (SM 5.0, entry `main`) and caches the binary.
    bool Compile(ID3D11Device* dev, const std::wstring& file, const char* entry = "main");

    // Recompiles only if the file's mtime changed. Returns:
    //  -1 unchanged, 0 = recompiled OK, <0 = error (previous binary kept).
    int  RecompileIfChanged(ID3D11Device* dev);

    // One-liner dispatch: SRVs in, one UAV out, params constant buffer.
    void Dispatch(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* const* srvs, UINT srvCount,
                  ID3D11UnorderedAccessView* uav, const ShaderParamsCB& params,
                  ID3D11Buffer* paramsCB);

    ID3D11ComputeShader* Get() const { return cs_.Get(); }
    const std::filesystem::path& Path() const { return path_; }
    const std::string& LastError() const { return lastError_; }

private:
    CPtr<ID3D11ComputeShader> cs_;
    std::filesystem::path     path_;
    std::filesystem::file_time_type mtime_{};
    std::string               lastError_;
};

} // namespace fs
