// d3dcompiler.h stub — compile surface only.
#pragma once
#include <windows.h>
#include <d3d11.h>

struct ID3DBlob : IUnknown {
    virtual void* GetBufferPointer() { return nullptr; }
    virtual SIZE_T GetBufferSize() { return 0; }
};

struct D3D_SHADER_MACRO { const char* Name; const char* Definition; };
struct ID3DInclude {};

enum : UINT { D3DCOMPILE_OPTIMIZATION_LEVEL3 = (1 << 16) | (1 << 15) | (1 << 14),
              D3DCOMPILE_DEBUG = 1, D3DCOMPILE_SKIP_OPTIMIZATION = 256 };

inline HRESULT D3DCompile(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
                          const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude,
                          LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2,
                          ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs) {
    (void)pSrcData; (void)SrcDataSize; (void)pSourceName; (void)pDefines; (void)pInclude;
    (void)pEntrypoint; (void)pTarget; (void)Flags1; (void)Flags2; (void)ppCode; (void)ppErrorMsgs;
    return E_NOTIMPL;
}

FS_UUID(ID3DBlob, 0x8ba5fb08, 0x5195, 0x40e2, 0xac,0x58,0x0d,0x98,0x9c,0x3a,0x01,0x02)
