// d3d11.h stub — compile surface only, real signatures.
#pragma once
#include <windows.h>
#include <dxgi.h>

typedef enum D3D_DRIVER_TYPE {
    D3D_DRIVER_TYPE_UNKNOWN = 0, D3D_DRIVER_TYPE_HARDWARE = 1, D3D_DRIVER_TYPE_WARP = 5,
} D3D_DRIVER_TYPE;

typedef enum D3D_FEATURE_LEVEL {
    D3D_FEATURE_LEVEL_9_1 = 0x9100, D3D_FEATURE_LEVEL_10_0 = 0xa000,
    D3D_FEATURE_LEVEL_10_1 = 0xa100, D3D_FEATURE_LEVEL_11_0 = 0xb000,
    D3D_FEATURE_LEVEL_11_1 = 0xb100,
} D3D_FEATURE_LEVEL;

typedef enum D3D11_USAGE { D3D11_USAGE_DEFAULT = 0, D3D11_USAGE_IMMUTABLE = 1,
                           D3D11_USAGE_DYNAMIC = 2, D3D11_USAGE_STAGING = 3 } D3D11_USAGE;

enum : UINT {
    D3D11_BIND_VERTEX_BUFFER       = 0x1L,
    D3D11_BIND_INDEX_BUFFER        = 0x2L,
    D3D11_BIND_CONSTANT_BUFFER     = 0x4L,
    D3D11_BIND_SHADER_RESOURCE     = 0x8L,
    D3D11_BIND_STREAM_OUTPUT       = 0x10L,
    D3D11_BIND_RENDER_TARGET       = 0x20L,
    D3D11_BIND_DEPTH_STENCIL       = 0x40L,
    D3D11_BIND_UNORDERED_ACCESS    = 0x80L,
};
enum : UINT {
    D3D11_CPU_ACCESS_WRITE = 0x10000L,
    D3D11_CPU_ACCESS_READ  = 0x20000L,
};
enum : UINT {
    D3D11_CREATE_DEVICE_SINGLETHREADED = 0x1L,
    D3D11_CREATE_DEVICE_DEBUG          = 0x2L,
    D3D11_CREATE_DEVICE_BGRA_SUPPORT   = 0x20L,
};
enum D3D11_MAP { D3D11_MAP_READ = 1, D3D11_MAP_WRITE = 2, D3D11_MAP_WRITE_DISCARD = 4 };
#define D3D11_SDK_VERSION 7
#define D3D11_FLOAT32_MAX 3.402823466e+38f

typedef enum D3D11_FILTER {
    D3D11_FILTER_MIN_MAG_MIP_POINT = 0, D3D11_FILTER_MIN_MAG_MIP_LINEAR = 0x15,
} D3D11_FILTER;
typedef enum D3D11_TEXTURE_ADDRESS_MODE {
    D3D11_TEXTURE_ADDRESS_WRAP = 1, D3D11_TEXTURE_ADDRESS_CLAMP = 2,
} D3D11_TEXTURE_ADDRESS_MODE;
typedef enum D3D11_COMPARISON_FUNC {
    D3D11_COMPARISON_NEVER = 1, D3D11_COMPARISON_ALWAYS = 8,
} D3D11_COMPARISON_FUNC;
typedef enum D3D11_SRV_DIMENSION {
    D3D11_SRV_DIMENSION_UNKNOWN = 0, D3D11_SRV_DIMENSION_BUFFER = 1,
    D3D11_SRV_DIMENSION_TEXTURE2D = 4,
} D3D11_SRV_DIMENSION;
typedef enum D3D11_UAV_DIMENSION {
    D3D11_UAV_DIMENSION_UNKNOWN = 0, D3D11_UAV_DIMENSION_BUFFER = 1,
    D3D11_UAV_DIMENSION_TEXTURE2D = 4,
} D3D11_UAV_DIMENSION;

typedef struct D3D11_TEXTURE2D_DESC {
    UINT Width; UINT Height; UINT MipLevels; UINT ArraySize;
    DXGI_FORMAT Format; DXGI_SAMPLE_DESC SampleDesc; D3D11_USAGE Usage;
    UINT BindFlags; UINT CPUAccessFlags; UINT MiscFlags;
} D3D11_TEXTURE2D_DESC;

typedef struct D3D11_BUFFER_DESC {
    UINT ByteWidth; D3D11_USAGE Usage; UINT BindFlags; UINT CPUAccessFlags; UINT MiscFlags;
    UINT StructureByteStride;
} D3D11_BUFFER_DESC;

typedef struct D3D11_SAMPLER_DESC {
    D3D11_FILTER Filter;
    D3D11_TEXTURE_ADDRESS_MODE AddressU; D3D11_TEXTURE_ADDRESS_MODE AddressV;
    D3D11_TEXTURE_ADDRESS_MODE AddressW;
    FLOAT MipLODBias; UINT MaxAnisotropy; D3D11_COMPARISON_FUNC ComparisonFunc;
    FLOAT BorderColor[4]; FLOAT MinLOD; FLOAT MaxLOD;
} D3D11_SAMPLER_DESC;

typedef struct D3D11_MAPPED_SUBRESOURCE {
    void* pData; UINT RowPitch; UINT DepthPitch;
} D3D11_MAPPED_SUBRESOURCE;

typedef struct D3D11_SUBRESOURCE_DATA {
    const void* pSysMem; UINT SysMemPitch; UINT SysMemSlicePitch;
} D3D11_SUBRESOURCE_DATA;

typedef struct D3D11_SHADER_RESOURCE_VIEW_DESC {
    DXGI_FORMAT Format; D3D11_SRV_DIMENSION ViewDimension;
    struct { UINT MostDetailedMip; UINT MipLevels; } Texture2D;
} D3D11_SHADER_RESOURCE_VIEW_DESC;

typedef struct D3D11_UNORDERED_ACCESS_VIEW_DESC {
    DXGI_FORMAT Format; D3D11_UAV_DIMENSION ViewDimension;
    struct { UINT MipSlice; } Texture2D;
} D3D11_UNORDERED_ACCESS_VIEW_DESC;

// ------------------------------------------------------------ interfaces --
struct ID3D11Device;
struct ID3D11DeviceChild : IUnknown {
    virtual void GetDevice(ID3D11Device** ppDevice) { (void)ppDevice; }
};
struct ID3D11Resource : ID3D11DeviceChild {};

struct ID3D11Buffer : ID3D11Resource {
    virtual void GetDesc(D3D11_BUFFER_DESC* pDesc) { (void)pDesc; }
};
struct ID3D11Texture2D : ID3D11Resource {
    virtual void GetDesc(D3D11_TEXTURE2D_DESC* pDesc) { (void)pDesc; }
};
struct ID3D11SamplerState : ID3D11DeviceChild {};
struct ID3D11ShaderResourceView : ID3D11DeviceChild {};
struct ID3D11UnorderedAccessView : ID3D11DeviceChild {};
struct ID3D11RenderTargetView : ID3D11DeviceChild {};
struct ID3D11ComputeShader : ID3D11DeviceChild {};
struct ID3D11VertexShader : ID3D11DeviceChild {};
struct ID3D11PixelShader : ID3D11DeviceChild {};
struct ID3D11InputLayout : ID3D11DeviceChild {};
struct ID3D11ClassLinkage : ID3D11DeviceChild {};
struct ID3D11BlendState : ID3D11DeviceChild {};
struct ID3D11Query : ID3D11DeviceChild {};

struct ID3D11DeviceContext : ID3D11DeviceChild {
    virtual void CopyResource(ID3D11Resource* pDst, ID3D11Resource* pSrc) { (void)pDst; (void)pSrc; }
    virtual HRESULT Map(ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags,
                        D3D11_MAPPED_SUBRESOURCE* pMappedResource) {
        (void)pResource; (void)Subresource; (void)MapType; (void)MapFlags; (void)pMappedResource; return E_NOTIMPL;
    }
    virtual void Unmap(ID3D11Resource* pResource, UINT Subresource) { (void)pResource; (void)Subresource; }
    virtual void CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
        (void)StartSlot; (void)NumBuffers; (void)ppConstantBuffers;
    }
    virtual void CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) {
        (void)StartSlot; (void)NumViews; (void)ppShaderResourceViews;
    }
    virtual void CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
                                           const UINT* pUAVInitialCounts) {
        (void)StartSlot; (void)NumUAVs; (void)ppUnorderedAccessViews; (void)pUAVInitialCounts;
    }
    virtual void CSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers) {
        (void)StartSlot; (void)NumSamplers; (void)ppSamplers;
    }
    virtual void CSSetShader(ID3D11ComputeShader* pShader, ID3D11ClassLinkage* const* ppClassInstances, UINT NumClassInstances) {
        (void)pShader; (void)ppClassInstances; (void)NumClassInstances;
    }
    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) {
        (void)ThreadGroupCountX; (void)ThreadGroupCountY; (void)ThreadGroupCountZ;
    }
};

struct ID3D11Device : IUnknown {
    virtual HRESULT CreateBuffer(const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData,
                                 ID3D11Buffer** ppBuffer) {
        (void)pDesc; (void)pInitialData; (void)ppBuffer; return E_NOTIMPL;
    }
    virtual HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData,
                                    ID3D11Texture2D** ppTexture2D) {
        (void)pDesc; (void)pInitialData; (void)ppTexture2D; return E_NOTIMPL;
    }
    virtual HRESULT CreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
                                             ID3D11ShaderResourceView** ppSRView) {
        (void)pResource; (void)pDesc; (void)ppSRView; return E_NOTIMPL;
    }
    virtual HRESULT CreateUnorderedAccessView(ID3D11Resource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
                                              ID3D11UnorderedAccessView** ppUAView) {
        (void)pResource; (void)pDesc; (void)ppUAView; return E_NOTIMPL;
    }
    virtual HRESULT CreateSamplerState(const D3D11_SAMPLER_DESC* pSamplerDesc, ID3D11SamplerState** ppSamplerState) {
        (void)pSamplerDesc; (void)ppSamplerState; return E_NOTIMPL;
    }
    virtual HRESULT CreateComputeShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
                                        ID3D11ClassLinkage* pClassLinkage, ID3D11ComputeShader** ppComputeShader) {
        (void)pShaderBytecode; (void)BytecodeLength; (void)pClassLinkage; (void)ppComputeShader; return E_NOTIMPL;
    }
    virtual void GetImmediateContext(ID3D11DeviceContext** ppImmediateContext) { (void)ppImmediateContext; }
};

inline HRESULT D3D11CreateDevice(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software,
                                 UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels,
                                 UINT SDKVersion, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel,
                                 ID3D11DeviceContext** ppImmediateContext) {
    (void)pAdapter; (void)DriverType; (void)Software; (void)Flags; (void)pFeatureLevels;
    (void)FeatureLevels; (void)SDKVersion; (void)ppDevice; (void)pFeatureLevel; (void)ppImmediateContext;
    return E_NOTIMPL;
}

FS_UUID(ID3D11Device,                 0xdb6f6ddb, 0xac77, 0x4e88, 0x82,0x53,0x81,0x9d,0xf9,0xbb,0xf1,0x40)
FS_UUID(ID3D11DeviceContext,          0xc0bfa96c, 0xe089, 0x44fb, 0x8e,0xaf,0x26,0xf8,0x79,0x61,0x90,0xda)
FS_UUID(ID3D11Texture2D,              0x6f15aaf2, 0xd208, 0x4e89, 0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c)
FS_UUID(ID3D11Buffer,                 0x48570b85, 0xd1ee, 0x4f8a, 0xa8,0x8b,0xa9,0xc4,0xbf,0xf2,0x23,0xf6)
FS_UUID(ID3D11SamplerState,           0xda6fea51, 0x564c, 0x4487, 0xa1,0x4b,0x5e,0x4f,0xa2,0xd0,0x93,0x0c)
FS_UUID(ID3D11ShaderResourceView,     0x6fbdc329, 0xd4f4, 0x41ca, 0x91,0x8e,0xef,0x0a,0x13,0x4c,0x8c,0x98)
FS_UUID(ID3D11UnorderedAccessView,    0x2844d9b3, 0x5fc9, 0x4ed3, 0xbf,0x5d,0xfe,0x92,0x0a,0x64,0x5c,0x8c)
FS_UUID(ID3D11ComputeShader,          0x4f5b196e, 0xc2bd, 0x495e, 0xbd,0x1d,0xef,0x87,0xf6,0x3c,0x49,0x02)
