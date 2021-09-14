// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "VARIDRendering.h"
#include "VARIDProfile.h"
#include "VARIDModule.h"

#include "CoreMinimal.h"
#include "EngineMinimal.h"
#include "EngineModule.h"
#include "EngineGlobals.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Containers/Array.h"
#include "StereoRendering.h"
#include "RenderGraph.h"
#include "RenderGraphResources.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "RendererInterface.h"
#include "RHICommandList.h"
#include "RHIStaticStates.h"
#include "ScreenPass.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameterStruct.h"
#include "ShaderPermutation.h"
#include "UniformBuffer.h"
#include "PipelineStateCache.h"
#include "ScreenRendering.h"
#include "PostProcess/SceneRenderTargets.h"
#include "CommonRenderResources.h"
#include "PixelShaderUtils.h"
#include "PostProcessMaterial.h"
#include "Math/UnrealMathUtility.h"
#include "SystemTextures.h"


static const int32 MAX_NUM_POINTS = 256;
static const uint8 MAX_NUM_MIP_LEVELS = 10;


struct FShaderParameterMapPoint
{
public:
	float X;
	float Y;
	float Value;
	float Padding;
};


static_assert(sizeof(FShaderParameterMapPoint) == 16, "FShaderParameterMapPoint is wrong size. Expected 16 byte alignment. Has it been changed?!");


class FQuadVertexBufferFull : public FVertexBuffer
{
public:

	void InitRHI()
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0].Position = FVector4(-1, 1, 0, 1);
		Vertices[0].UV = FVector2D(0, 0);

		Vertices[1].Position = FVector4(1, 1, 0, 1);
		Vertices[1].UV = FVector2D(1, 0);

		Vertices[2].Position = FVector4(-1, -1, 0, 1);
		Vertices[2].UV = FVector2D(0, 1);

		Vertices[3].Position = FVector4(1, -1, 0, 1);
		Vertices[3].UV = FVector2D(1, 1);

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};
TGlobalResource<FQuadVertexBufferFull> GQuadVertexBufferFull;

/** note UVs on x axis go from 0 to 0.5*/
class FQuadVertexBufferLeft : public FVertexBuffer
{
public:

	void InitRHI()
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0].Position = FVector4(-1, 1, 0, 1);
		Vertices[0].UV = FVector2D(0, 0);

		Vertices[1].Position = FVector4(1, 1, 0, 1);
		Vertices[1].UV = FVector2D(0.5, 0);

		Vertices[2].Position = FVector4(-1, -1, 0, 1);
		Vertices[2].UV = FVector2D(0, 1);

		Vertices[3].Position = FVector4(1, -1, 0, 1);
		Vertices[3].UV = FVector2D(0.5, 1);

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};
TGlobalResource<FQuadVertexBufferLeft> GQuadVertexBufferLeft;

/** note UVs on x axis go from 0.5 to 1*/
class FQuadVertexBufferRight : public FVertexBuffer
{
public:

	void InitRHI()
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0].Position = FVector4(-1, 1, 0, 1);
		Vertices[0].UV = FVector2D(0.5, 0);

		Vertices[1].Position = FVector4(1, 1, 0, 1);
		Vertices[1].UV = FVector2D(1, 0);

		Vertices[2].Position = FVector4(-1, -1, 0, 1);
		Vertices[2].UV = FVector2D(0.5, 1);

		Vertices[3].Position = FVector4(1, -1, 0, 1);
		Vertices[3].UV = FVector2D(1, 1);

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};
TGlobalResource<FQuadVertexBufferRight> GQuadVertexBufferRight;


class FVARIDQuadVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVARIDQuadVS, Global);

public:

	SHADER_USE_PARAMETER_STRUCT(FVARIDQuadVS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDQuadVS, "/Plugin/VARID/Private/VARIDQuadVS.usf", "MainVS", SF_Vertex);


class FVARIDQuadPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVARIDQuadPS, Global);

public:

	SHADER_USE_PARAMETER_STRUCT(FVARIDQuadPS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InGaussianSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InLaplacianSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InContrastSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InInpaintSRV)

		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InBlurVFMapSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InContrastVFMapSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InInpaintVFMapSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InWarpVFMapSRV)

		SHADER_PARAMETER_SAMPLER(SamplerState, InBilinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, InTrilinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, InPointSampler)

		SHADER_PARAMETER(float, InMaxMipLevel)

		RENDER_TARGET_BINDING_SLOTS()

		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDQuadPS, "/Plugin/VARID/Private/VARIDQuadPS.usf", "MainPS", SF_Pixel);


class FVARIDBasicResampleCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDBasicResampleCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDBasicResampleCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, InDispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, InTexelSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSampler)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDBasicResampleCS, "/Plugin/VARID/Private/VARIDBasicResampleCS.usf", "MainCS", SF_Compute);


class FVARIDLaplacianCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDLaplacianCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDLaplacianCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, DispatchThreadIDOffset)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InLoResSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InHiResSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutLaplacianUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDLaplacianCS, "/Plugin/VARID/Private/VARIDLaplacianCS.usf", "MainCS", SF_Compute);


class FVARIDGaussianBlurCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDGaussianBlurCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDGaussianBlurCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, DispatchThreadIDOffset)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDGaussianBlurCS, "/Plugin/VARID/Private/VARIDGaussianBlurCS.usf", "MainCS", SF_Compute);


class FVARIDReconstructCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDReconstructCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDReconstructCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, InDispatchThreadIDOffset)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InLaplacianSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InGaussianSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InVFMapSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDReconstructCS, "/Plugin/VARID/Private/VARIDContrastReconstructCS.usf", "MainCS", SF_Compute)


class FVARIDInpainterInitialiseCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDInpainterInitialiseCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDInpainterInitialiseCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, InDispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, InTexelSize)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InMaskSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutMetaDataUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDInpainterInitialiseCS, "/Plugin/VARID/Private/VARIDInpainterInitialiseCS.usf", "MainCS", SF_Compute)


class FVARIDInpainterFillCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDInpainterFillCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDInpainterFillCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, InDispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, InTexelSize)
		SHADER_PARAMETER(int32, PassCounter)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InColourSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InMaskSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InMetaDataSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutMetaDataUAV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutColourUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDInpainterFillCS, "/Plugin/VARID/Private/VARIDInpainterFillCS.usf", "MainCS", SF_Compute)


class FVARIDInpainterFinaliseCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDInpainterFinaliseCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDInpainterFinaliseCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, InDispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, InTexelSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, InBilinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, InPointSampler)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InColourSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InMaskSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InMetaDataSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutPositionUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDInpainterFinaliseCS, "/Plugin/VARID/Private/VARIDInpainterFinaliseCS.usf", "MainCS", SF_Compute)


class FVARIDHeightMapCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDHeightMapCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDHeightMapCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, DispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, TexelSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FShaderParameterMapPoint>, VFMapPoints)
		SHADER_PARAMETER(uint32, NumVFMapPoints)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutUAV)
		SHADER_PARAMETER(float, InOriginOffset)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("MAX_NUM_POINTS"), MAX_NUM_POINTS);
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDHeightMapCS, "/Plugin/VARID/Private/VARIDHeightMapCS.usf", "MainCS", SF_Compute);


class FVARIDNormalMapCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDNormalMapCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDNormalMapCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, DispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, TexelSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, OutUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDNormalMapCS, "/Plugin/VARID/Private/VARIDNormalMapCS.usf", "MainCS", SF_Compute);


class FVARIDPositionMapCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDPositionMapCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDPositionMapCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, DispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, TexelSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FShaderParameterMapPoint>, VFMapPoints)
		SHADER_PARAMETER(uint32, NumVFMapPoints)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutUAV)
		SHADER_PARAMETER(float, InOriginOffset)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDPositionMapCS, "/Plugin/VARID/Private/VARIDPositionMapCS.usf", "MainCS", SF_Compute);


class FVARIDDirectCopyCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDDirectCopyCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDDirectCopyCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, DispatchThreadIDOffset)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDDirectCopyCS, "/Plugin/VARID/Private/VARIDDirectCopyCS.usf", "MainCS", SF_Compute);


class FVARIDDirectCopyMaskedCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDDirectCopyMaskedCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDDirectCopyMaskedCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, DispatchThreadIDOffset)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InMaskSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDDirectCopyMaskedCS, "/Plugin/VARID/Private/VARIDDirectCopyMaskedCS.usf", "MainCS", SF_Compute);





// PIXMIX inspired shaders - not currently used

class FVARIDInpainterPropagateCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDInpainterPropagateCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDInpainterPropagateCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, InDispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, InTexelSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, InBilinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, InPointSampler)
		SHADER_PARAMETER(FIntPoint, InTextureSize)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InColourSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InMaskSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float2>, InPositionCostSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, OutPositionUAV)

		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDInpainterPropagateCS, "/Plugin/VARID/Private/VARIDInpainterPropagateCS.usf", "MainCS", SF_Compute)


class FVARIDInpainterRandomPositionCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDInpainterRandomPositionCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDInpainterRandomPositionCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, InDispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, InTexelSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, InBilinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, InPointSampler)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InMaskSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, OutPositionUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDInpainterRandomPositionCS, "/Plugin/VARID/Private/VARIDInpainterRandomPositionCS.usf", "MainCS", SF_Compute)


class FVARIDInpainterCostCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVARIDInpainterCostCS)
	SHADER_USE_PARAMETER_STRUCT(FVARIDInpainterCostCS, FGlobalShader)

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntPoint, InDispatchThreadIDOffset)
		SHADER_PARAMETER(FVector2D, InTexelSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, InBilinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, InPointSampler)
		SHADER_PARAMETER(FIntPoint, InTextureSize)
		SHADER_PARAMETER(float, InFloatMax)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InColourSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InMaskSRV)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float2>, InPositionSRV)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutPositionCostUAV)
		END_SHADER_PARAMETER_STRUCT();

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVARIDInpainterCostCS, "/Plugin/VARID/Private/VARIDInpainterCostCS.usf", "MainCS", SF_Compute)

/*****************************************************************************************************************/
// VF map

/** even if we have no points to pass in, we still generate a texture. in the case of zero points the texture would be black */
static bool BuildHeightMapTexture_RenderThread
(
	FRDGBuilder& InGraphBuilder,
	const bool InFXEnabled,
	const TArray<FVARIDVFMapPoint>& InVFMapPoints,
	const int32 InMipLevel,
	const FVector2D& InEyeGazePoint,
	float InOriginOffset,
	FRDGTextureRef OutHeightMapTexture,
	const FIntRect& InViewportRect,
	const EStereoscopicPass InStereoPass,
	const bool InFullField
)
{
	check(InMipLevel >= 0);
	check(OutHeightMapTexture);

	const FRDGTextureDesc& OutHeightMapTextureDesc = OutHeightMapTexture->Desc;
	const FIntPoint TextureSize(FMath::Max(OutHeightMapTextureDesc.Extent.X >> InMipLevel, 1), FMath::Max(OutHeightMapTextureDesc.Extent.Y >> InMipLevel, 1));
	const FVector2D TexelSize(1.0f / TextureSize.X, 1.0f / TextureSize.Y);
	const FIntPoint DispatchSize(FMath::Max(InViewportRect.Width() >> InMipLevel, 1), FMath::Max(InViewportRect.Height() >> InMipLevel, 1));
	const FIntPoint DispatchThreadIDOffset(InViewportRect.Min.X >> InMipLevel, InViewportRect.Min.Y >> InMipLevel);

	TArray<FShaderParameterMapPoint> FilteredPoints;

	if (InFXEnabled)
	{
		if (InFullField && InVFMapPoints.Num() == 1)
		{
			InOriginOffset = InVFMapPoints[0].NormValue;
			// FilteredPoints will be left empty
		}
		else
		{
			float XScale = 1.0f;
			float XOffset = 0.0f;

			switch (InStereoPass)
			{
			case eSSP_FULL:
				break;
			case eSSP_LEFT_EYE:
				XScale = 0.5f;
				break;
			case eSSP_RIGHT_EYE:
				XScale = 0.5f;
				XOffset = 0.5f;
				break;
			case eSSP_LEFT_EYE_SIDE:
				break;
			case eSSP_RIGHT_EYE_SIDE:
				break;
			default:
				break;
			}

			for (int32 i = 0; i < InVFMapPoints.Num(); ++i)
			{
				FShaderParameterMapPoint P;
				P.X = ((InVFMapPoints[i].NormX + InEyeGazePoint.X) * XScale) + XOffset;
				P.Y = (InVFMapPoints[i].NormY + InEyeGazePoint.Y);
				P.Value = InVFMapPoints[i].NormValue;
				P.Padding = 1.0f;	// makes the struct have 16 byte alignment
				FilteredPoints.Add(P);
			}
		}
	}

	// Even if there are no points Keep going - still need to generate a texture as the remaining parts of the render pipeline are relying on a valid texture to exist.

	TShaderMapRef<FVARIDHeightMapCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FVARIDHeightMapCS::FParameters* PassParameters = InGraphBuilder.AllocParameters<FVARIDHeightMapCS::FParameters>();
	PassParameters->DispatchThreadIDOffset = DispatchThreadIDOffset;
	PassParameters->TexelSize = TexelSize;
	PassParameters->LinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->PointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->InOriginOffset = InOriginOffset;	// intensity origin
	PassParameters->NumVFMapPoints = FilteredPoints.Num();
	if (FilteredPoints.Num() > 0)
	{
		PassParameters->VFMapPoints = InGraphBuilder.CreateSRV(CreateStructuredBuffer(InGraphBuilder, TEXT("VFMapPoints"), sizeof(FShaderParameterMapPoint), FilteredPoints.Num(), FilteredPoints.GetData(), sizeof(FShaderParameterMapPoint) * FilteredPoints.Num(), ERDGInitialDataFlags::None));
	}
	else
	{
		// HACK cant pass array with size zero. Have to pass at least one element. 
		FShaderParameterMapPoint DummyPoint;
		PassParameters->VFMapPoints = InGraphBuilder.CreateSRV(CreateStructuredBuffer(InGraphBuilder, TEXT("VFMapPoints"), sizeof(FShaderParameterMapPoint), 1, &DummyPoint, sizeof(FShaderParameterMapPoint), ERDGInitialDataFlags::None));
	}
	PassParameters->OutUAV = InGraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutHeightMapTexture, InMipLevel));

	FComputeShaderUtils::AddPass(
		InGraphBuilder,
		RDG_EVENT_NAME("VARID - Build Height Map - MipLevel=%d", InMipLevel),
		ComputeShader,
		PassParameters,
		FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));

	return true;
}

static bool BuildNormalMapTexture_RenderThread
(
	FRDGBuilder& InGraphBuilder,
	const bool InFXEnabled,
	const TArray<FVARIDVFMapPoint>& InVFMapPoints,
	const FVector2D& InEyeGazePoint,
	const float InOriginOffset,
	FRDGTextureRef OutNormalMapTexture,
	const FIntRect& ViewportRect,
	const EStereoscopicPass InStereoPass,
	const bool InFullField
)
{
	check(OutNormalMapTexture);

	const FRDGTextureDesc& TextureDesc = OutNormalMapTexture->Desc;

	FRDGTextureRef HeightMapTexture = InGraphBuilder.CreateTexture(TextureDesc, TEXT("HeightMapTexture"));
	if (!BuildHeightMapTexture_RenderThread(InGraphBuilder, InFXEnabled, InVFMapPoints, 0, InEyeGazePoint, InOriginOffset, HeightMapTexture, ViewportRect, InStereoPass, InFullField))
	{
		return false;
	}

	TShaderMapRef<FVARIDNormalMapCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FVARIDNormalMapCS::FParameters* PassParameters = InGraphBuilder.AllocParameters<FVARIDNormalMapCS::FParameters>();
	PassParameters->DispatchThreadIDOffset = ViewportRect.Min;
	PassParameters->TexelSize = FVector2D(1.0f / TextureDesc.Extent.X, 1.0f / TextureDesc.Extent.Y);
	PassParameters->LinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->PointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->InSRV = InGraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(HeightMapTexture, 0));
	PassParameters->OutUAV = InGraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutNormalMapTexture, 0));

	FComputeShaderUtils::AddPass(
		InGraphBuilder,
		RDG_EVENT_NAME("VARID BuildNormalMapTexture"),
		ComputeShader,
		PassParameters,
		FComputeShaderUtils::GetGroupCount(ViewportRect.Size(), FComputeShaderUtils::kGolden2DGroupSize));

	return true;
}

static bool BuildPositionMapTexture_RenderThread
(
	FRDGBuilder& InGraphBuilder,
	const bool InFXEnabled,
	const TArray<FVARIDVFMapPoint>& InVFMapPoints,
	const int32 InMipLevel,
	const FVector2D& InEyeGazePoint,
	float InOriginOffset,
	FRDGTextureRef OutTexture,
	const FIntRect& InViewportRect,
	const EStereoscopicPass InStereoPass,
	const bool InFullField
)
{
	check(InMipLevel >= 0);
	check(OutTexture);

	const FRDGTextureDesc& OutTextureDesc = OutTexture->Desc;
	const FIntPoint TextureSize(FMath::Max(OutTextureDesc.Extent.X >> InMipLevel, 1), FMath::Max(OutTextureDesc.Extent.Y >> InMipLevel, 1));
	const FVector2D TexelSize(1.0f / TextureSize.X, 1.0f / TextureSize.Y);
	const FIntPoint DispatchSize(FMath::Max(InViewportRect.Width() >> InMipLevel, 1), FMath::Max(InViewportRect.Height() >> InMipLevel, 1));
	const FIntPoint DispatchThreadIDOffset(InViewportRect.Min.X >> InMipLevel, InViewportRect.Min.Y >> InMipLevel);

	TArray<FShaderParameterMapPoint> FilteredPoints;

	if (InFXEnabled)
	{
		if (InFullField && InVFMapPoints.Num() == 1)
		{
			InOriginOffset = InVFMapPoints[0].NormValue;
			// FilteredPoints will be left empty
		}
		else
		{
			float XScale = 1.0f;
			float XOffset = 0.0f;

			switch (InStereoPass)
			{
			case eSSP_FULL:
				break;
			case eSSP_LEFT_EYE:
				XScale = 0.5f;
				break;
			case eSSP_RIGHT_EYE:
				XScale = 0.5f;
				XOffset = 0.5f;
				break;
			case eSSP_LEFT_EYE_SIDE:
				break;
			case eSSP_RIGHT_EYE_SIDE:
				break;
			default:
				break;
			}

			for (int32 i = 0; i < InVFMapPoints.Num(); ++i)
			{
				FShaderParameterMapPoint P;
				P.X = ((InVFMapPoints[i].NormX + InEyeGazePoint.X) * XScale) + XOffset;
				P.Y = (InVFMapPoints[i].NormY + InEyeGazePoint.Y);
				P.Value = InVFMapPoints[i].NormValue;
				P.Padding = 1.0f;	// makes the struct have 16 byte alignment
				FilteredPoints.Add(P);
			}
		}
	}

	// Even if there are no points Keep going - still need to generate a texture as the remaining parts of the render pipeline are relying on a valid texture to exist.

	TShaderMapRef<FVARIDPositionMapCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FVARIDPositionMapCS::FParameters* PassParameters = InGraphBuilder.AllocParameters<FVARIDPositionMapCS::FParameters>();
	PassParameters->DispatchThreadIDOffset = DispatchThreadIDOffset;
	PassParameters->TexelSize = TexelSize;
	PassParameters->LinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->PointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->InOriginOffset = InOriginOffset;	// intensity origin
	PassParameters->NumVFMapPoints = FilteredPoints.Num();
	if (FilteredPoints.Num() > 0)
	{
		PassParameters->VFMapPoints = InGraphBuilder.CreateSRV(CreateStructuredBuffer(InGraphBuilder, TEXT("VFMapPoints"), sizeof(FShaderParameterMapPoint), FilteredPoints.Num(), FilteredPoints.GetData(), sizeof(FShaderParameterMapPoint) * FilteredPoints.Num(), ERDGInitialDataFlags::None));
	}
	else
	{
		// HACK cant pass array with size zero. Have to pass at least one element. 
		FShaderParameterMapPoint DummyPoint;
		PassParameters->VFMapPoints = InGraphBuilder.CreateSRV(CreateStructuredBuffer(InGraphBuilder, TEXT("VFMapPoints"), sizeof(FShaderParameterMapPoint), 1, &DummyPoint, sizeof(FShaderParameterMapPoint), ERDGInitialDataFlags::None));
	}
	PassParameters->OutUAV = InGraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutTexture, InMipLevel));

	FComputeShaderUtils::AddPass(
		InGraphBuilder,
		RDG_EVENT_NAME("VARID - Build Position Map - MipLevel=%d", InMipLevel),
		ComputeShader,
		PassParameters,
		FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));

	return true;
}

/*****************************************************************************************************************/
// FX 

static bool BuildPyramid_RenderThread(FRDGBuilder& GraphBuilder, FRDGTextureRef Texture, FRHISamplerState* Sampler, const FIntRect& ViewportRect)
{
	check(Texture);

	const FRDGTextureDesc& TextureDesc = Texture->Desc;
	TShaderMapRef<FVARIDBasicResampleCS> ResampleComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	for (uint32 MipLevel = 1; MipLevel < TextureDesc.NumMips; ++MipLevel)	// NOTE start at level 1 (level zero should already exist!)
	{
		const FIntPoint TextureSize(FMath::Max(TextureDesc.Extent.X >> MipLevel, 1), FMath::Max(TextureDesc.Extent.Y >> MipLevel, 1));
		const FVector2D TexelSize(1.0f / TextureSize.X, 1.0f / TextureSize.Y);
		const FIntPoint DispatchSize(FMath::Max(ViewportRect.Width() >> MipLevel, 1), FMath::Max(ViewportRect.Height() >> MipLevel, 1));
		const FIntPoint DispatchThreadIDOffset(ViewportRect.Min.X >> MipLevel, ViewportRect.Min.Y >> MipLevel);

		FVARIDBasicResampleCS::FParameters* ResamplePassParameters = GraphBuilder.AllocParameters<FVARIDBasicResampleCS::FParameters>();
		ResamplePassParameters->InDispatchThreadIDOffset = DispatchThreadIDOffset;
		ResamplePassParameters->InTexelSize = TexelSize;
		ResamplePassParameters->InSampler = Sampler;
		ResamplePassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(Texture, MipLevel - 1));	// SRV is for a different mip level to the UAV - therefore Texture can appear to be both input and output
		ResamplePassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(Texture, MipLevel));

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("VARID - Build Pyramid - Downsample - MipLevel=%d", MipLevel),
			ResampleComputeShader,
			ResamplePassParameters,
			FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
	}

	return true;
}

static bool BuildPyramidCopy_RenderThread(FRDGBuilder& GraphBuilder, FRDGTextureRef InTexture, FRDGTextureRef OutTexture, FRHISamplerState* Sampler, const FIntRect& ViewportRect)
{
	check(InTexture);
	check(OutTexture);

	const FRDGTextureDesc& OutTextureDesc = OutTexture->Desc;
	TShaderMapRef<FVARIDBasicResampleCS> ResampleComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	for (uint32 MipLevel = 0; MipLevel < OutTextureDesc.NumMips; ++MipLevel)
	{
		const FIntPoint DestTextureSize(FMath::Max(OutTextureDesc.Extent.X >> MipLevel, 1), FMath::Max(OutTextureDesc.Extent.Y >> MipLevel, 1));
		const FVector2D TexelSize = FVector2D(1.0f / DestTextureSize.X, 1.0f / DestTextureSize.Y);
		const FIntPoint DispatchSize(FMath::Max(ViewportRect.Width() >> MipLevel, 1), FMath::Max(ViewportRect.Height() >> MipLevel, 1));
		const FIntPoint DispatchThreadIDOffset(ViewportRect.Min.X >> MipLevel, ViewportRect.Min.Y >> MipLevel);

		if (MipLevel == 0)
		{
			TShaderMapRef<FVARIDDirectCopyCS> DirectCopyComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FVARIDDirectCopyCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDDirectCopyCS::FParameters>();
			PassParameters->DispatchThreadIDOffset = DispatchThreadIDOffset;
			PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InTexture, 0));
			PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutTexture, 0));

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Direct Copy - MipLevel=%d", MipLevel),
				DirectCopyComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
		}
		else
		{
			FVARIDBasicResampleCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDBasicResampleCS::FParameters>();
			PassParameters->InDispatchThreadIDOffset = DispatchThreadIDOffset;
			PassParameters->InTexelSize = TexelSize;
			PassParameters->InSampler = Sampler;
			PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(OutTexture, MipLevel - 1));	// SRV is for a different mip level to the UAV - therefore Texture can appear to be both input and output
			PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutTexture, MipLevel));

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Build Pyramid Copy - Downsample - MipLevel=%d", MipLevel),
				ResampleComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
		}
	}

	return true;
}

static void BuildGaussianPyramid_RenderThread(FRDGBuilder& GraphBuilder, FRDGTextureRef InTexture, FRDGTextureRef OutGaussianMipTexture, const FIntRect& ViewportRect)
{
	check(InTexture);
	check(OutGaussianMipTexture);

	const FRDGTextureDesc& OutGaussianMipTextureDesc = OutGaussianMipTexture->Desc;

	FRDGTextureRef BlurredMipTexture = GraphBuilder.CreateTexture(OutGaussianMipTextureDesc, TEXT("VARID_TEMP_MipsRenderTargetTexture"));

	TShaderMapRef<FVARIDGaussianBlurCS> GaussianBlurComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDBasicResampleCS> ResampleComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	for (uint32 MipLevel = 0; MipLevel < OutGaussianMipTextureDesc.NumMips; ++MipLevel)
	{
		if (MipLevel == 0)
		{
			const FIntPoint DispatchSize = ViewportRect.Size();
			const FIntPoint DispatchThreadIDOffset = ViewportRect.Min;

			TShaderMapRef<FVARIDDirectCopyCS> DirectCopyComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			FVARIDDirectCopyCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDDirectCopyCS::FParameters>();
			PassParameters->DispatchThreadIDOffset = DispatchThreadIDOffset;
			PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InTexture, 0));
			PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutGaussianMipTexture, 0));

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Build Gaussian Pyramid - Direct Copy - MipLevel=%d", 0),
				DirectCopyComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
		}
		else
		{
			int32 LoResMipLevel = MipLevel;
			int32 HiResMipLevel = MipLevel - 1;

			const FIntPoint LoResTextureSize(FMath::Max(OutGaussianMipTextureDesc.Extent.X >> LoResMipLevel, 1), FMath::Max(OutGaussianMipTextureDesc.Extent.Y >> LoResMipLevel, 1));
			const FVector2D LoResTexelSize(1.0f / LoResTextureSize.X, 1.0f / LoResTextureSize.Y);

			const FIntPoint LoResDispatchSize(FMath::Max(ViewportRect.Width() >> LoResMipLevel, 1), FMath::Max(ViewportRect.Height() >> LoResMipLevel, 1));
			const FIntPoint LoResDispatchThreadIDOffset(ViewportRect.Min.X >> LoResMipLevel, ViewportRect.Min.Y >> LoResMipLevel);

			const FIntPoint HiResDispatchSize(FMath::Max(ViewportRect.Width() >> HiResMipLevel, 1), FMath::Max(ViewportRect.Height() >> HiResMipLevel, 1));
			const FIntPoint HiResDispatchThreadIDOffset(ViewportRect.Min.X >> HiResMipLevel, ViewportRect.Min.Y >> HiResMipLevel);

			// DONT downsample THEN Filter. Noise will alias back in. 
			// DO filter THEN downsample

			// blur
			FVARIDGaussianBlurCS::FParameters* GaussianBlurPassParameters = GraphBuilder.AllocParameters<FVARIDGaussianBlurCS::FParameters>();
			GaussianBlurPassParameters->DispatchThreadIDOffset = HiResDispatchThreadIDOffset;
			GaussianBlurPassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(OutGaussianMipTexture, HiResMipLevel));
			GaussianBlurPassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(BlurredMipTexture, HiResMipLevel));

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Build Gaussian Pyramid - Gaussian Blur - MipLevel=%d", HiResMipLevel),
				GaussianBlurComputeShader,
				GaussianBlurPassParameters,
				FComputeShaderUtils::GetGroupCount(HiResDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));


			// downsample
			FVARIDBasicResampleCS::FParameters* ResamplePassParameters = GraphBuilder.AllocParameters<FVARIDBasicResampleCS::FParameters>();
			ResamplePassParameters->InDispatchThreadIDOffset = LoResDispatchThreadIDOffset;
			ResamplePassParameters->InTexelSize = LoResTexelSize;
			ResamplePassParameters->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			ResamplePassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(BlurredMipTexture, HiResMipLevel));
			ResamplePassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutGaussianMipTexture, LoResMipLevel));

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Build Gaussian Pyramid - Downsample - MipLevel=%d", LoResMipLevel),
				ResampleComputeShader,
				ResamplePassParameters,
				FComputeShaderUtils::GetGroupCount(LoResDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
		}
	}
}

static void BuildLaplacianPyramid_RenderThread(FRDGBuilder& GraphBuilder, FRDGTextureRef InGaussianMipTexture, FRDGTextureRef OutLaplacianMipTexture, const FIntRect& ViewportRect)
{
	check(InGaussianMipTexture);
	check(OutLaplacianMipTexture);

	const FRDGTextureDesc& OutLaplacianMipTextureDesc = OutLaplacianMipTexture->Desc;

	uint32 MaxMipLevelIndex = InGaussianMipTexture->Desc.NumMips - 1;	// convert to zero based index

	FRDGTextureRef UpsampledMipTexture = GraphBuilder.CreateTexture(OutLaplacianMipTextureDesc, TEXT("VARID_TEMP_UpsampledMipTexture"));
	FRDGTextureRef BlurredMipTexture = GraphBuilder.CreateTexture(OutLaplacianMipTextureDesc, TEXT("VARID_TEMP_BlurredMipTexture"));

	TShaderMapRef<FVARIDBasicResampleCS> ResampleComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDGaussianBlurCS> GaussianBlurComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDLaplacianCS> LaplacianComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	// work from the highest mip level (lowest resolution) to the lowest mip level (highest resolution)
	for (int32 MipLevel = MaxMipLevelIndex; MipLevel >= 0; --MipLevel)
	{
		if (MipLevel == MaxMipLevelIndex)
		{
			const FIntPoint DispatchSize(FMath::Max(ViewportRect.Width() >> MipLevel, 1), FMath::Max(ViewportRect.Height() >> MipLevel, 1));
			const FIntPoint DispatchThreadIDOffset(ViewportRect.Min.X >> MipLevel, ViewportRect.Min.Y >> MipLevel);

			TShaderMapRef<FVARIDDirectCopyCS> DirectCopyComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			FVARIDDirectCopyCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDDirectCopyCS::FParameters>();
			PassParameters->DispatchThreadIDOffset = DispatchThreadIDOffset;
			PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InGaussianMipTexture, MipLevel));
			PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutLaplacianMipTexture, MipLevel));

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Build Laplacian Pyramid - Direct Copy - MipLevel=%d", MipLevel),
				DirectCopyComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
		}
		else
		{
			int32 LoResMipLevel = MipLevel + 1;
			int32 HiResMipLevel = MipLevel;

			const FIntPoint HiResTextureSize(FMath::Max(OutLaplacianMipTextureDesc.Extent.X >> HiResMipLevel, 1), FMath::Max(OutLaplacianMipTextureDesc.Extent.Y >> HiResMipLevel, 1));
			const FVector2D HiResTexelSize(1.0f / HiResTextureSize.X, 1.0f / HiResTextureSize.Y);

			const FIntPoint LoResDispatchSize(FMath::Max(ViewportRect.Width() >> LoResMipLevel, 1), FMath::Max(ViewportRect.Height() >> LoResMipLevel, 1));
			const FIntPoint LoResDispatchThreadIDOffset(ViewportRect.Min.X >> LoResMipLevel, ViewportRect.Min.Y >> LoResMipLevel);

			const FIntPoint HiResDispatchSize(FMath::Max(ViewportRect.Width() >> HiResMipLevel, 1), FMath::Max(ViewportRect.Height() >> HiResMipLevel, 1));
			const FIntPoint HiResDispatchThreadIDOffset(ViewportRect.Min.X >> HiResMipLevel, ViewportRect.Min.Y >> HiResMipLevel);

			{
				// upsample
				FVARIDBasicResampleCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDBasicResampleCS::FParameters>();
				PassParameters->InDispatchThreadIDOffset = HiResDispatchThreadIDOffset;
				PassParameters->InTexelSize = HiResTexelSize;
				PassParameters->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InGaussianMipTexture, LoResMipLevel));
				PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(UpsampledMipTexture, HiResMipLevel));

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Build Laplacian Pyramid - Upsample - MipLevel=%d", HiResMipLevel),
					ResampleComputeShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(HiResDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}

			{
				// blur
				FVARIDGaussianBlurCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDGaussianBlurCS::FParameters>();
				PassParameters->DispatchThreadIDOffset = HiResDispatchThreadIDOffset;
				PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(UpsampledMipTexture, HiResMipLevel));
				PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(BlurredMipTexture, HiResMipLevel));

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Build Laplacian Pyramid - Gaussian Blur - MipLevel=%d", HiResMipLevel),
					GaussianBlurComputeShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(HiResDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}

			{
				// laplacian
				FVARIDLaplacianCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDLaplacianCS::FParameters>();
				PassParameters->DispatchThreadIDOffset = HiResDispatchThreadIDOffset;
				PassParameters->InLoResSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(BlurredMipTexture, HiResMipLevel));
				PassParameters->InHiResSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InGaussianMipTexture, HiResMipLevel));
				PassParameters->OutLaplacianUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutLaplacianMipTexture, HiResMipLevel));

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Build Laplacian Pyramid - Laplacian - MipLevel=%d", HiResMipLevel),
					LaplacianComputeShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(HiResDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}
		}
	}
}

static void BuildContrastTexture_RenderThread(FRDGBuilder& GraphBuilder, FRDGTextureRef InLaplacianMipTexture, FRDGTextureRef InVFMapMipTexture, FRDGTextureRef OutContrastTexture, const FIntRect& ViewportRect)
{
	check(InLaplacianMipTexture);
	check(OutContrastTexture);

	const FRDGTextureDesc& OutContrastTextureDesc = OutContrastTexture->Desc;
	const int32 ContrastNumberOfMips = OutContrastTextureDesc.NumMips;

	const FRDGTextureDesc& InLaplacianMipTextureDesc = InLaplacianMipTexture->Desc;
	const int32 LaplacianNumberOfMips = InLaplacianMipTextureDesc.NumMips;

	const FRDGTextureDesc& InVFMapMipTextureDesc = InVFMapMipTexture->Desc;
	const int32 VFMapNumberOfMips = InVFMapMipTextureDesc.NumMips;

	check(ContrastNumberOfMips == LaplacianNumberOfMips);
	check(ContrastNumberOfMips == VFMapNumberOfMips);

	const int32 MaxMipLevelIndex = ContrastNumberOfMips - 1;

	FRDGTextureRef UpsampledMipTexture = GraphBuilder.CreateTexture(OutContrastTextureDesc, TEXT("VARID_TEMP_UpsampledMipTexture"));
	FRDGTextureRef BlurredMipTexture = GraphBuilder.CreateTexture(OutContrastTextureDesc, TEXT("VARID_TEMP_BlurredMipTexture"));

	TShaderMapRef<FVARIDBasicResampleCS> ResampleComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDGaussianBlurCS> GaussianBlurComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDReconstructCS> ReconstructComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	// work from the highest mip level (lowest resolution) to the lowest mip level (highest resolution)
	for (int32 MipLevel = MaxMipLevelIndex; MipLevel >= 0; --MipLevel)
	{
		// TODO move outside the for loop?
		if (MipLevel == MaxMipLevelIndex)
		{
			// lowest res level is simply a direct copy. no bias applied
			const FIntPoint DispatchSize(FMath::Max(ViewportRect.Width() >> MipLevel, 1), FMath::Max(ViewportRect.Height() >> MipLevel, 1));
			const FIntPoint DispatchThreadIDOffset(ViewportRect.Min.X >> MipLevel, ViewportRect.Min.Y >> MipLevel);

			TShaderMapRef<FVARIDDirectCopyCS> DirectCopyComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			FVARIDDirectCopyCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDDirectCopyCS::FParameters>();
			PassParameters->DispatchThreadIDOffset = DispatchThreadIDOffset;
			PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InLaplacianMipTexture, MipLevel));
			PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutContrastTexture, MipLevel));

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Build Contrast Texture - Direct Copy - MipLevel=%d", MipLevel),
				DirectCopyComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
		}
		else
		{
			int32 LoResMipLevel = MipLevel + 1;
			int32 HiResMipLevel = MipLevel;

			const FIntPoint HiResTextureSize(FMath::Max(OutContrastTextureDesc.Extent.X >> HiResMipLevel, 1), FMath::Max(OutContrastTextureDesc.Extent.Y >> HiResMipLevel, 1));
			const FVector2D HiResTexelSize(1.0f / HiResTextureSize.X, 1.0f / HiResTextureSize.Y);

			const FIntPoint HiResDispatchSize(FMath::Max(ViewportRect.Width() >> HiResMipLevel, 1), FMath::Max(ViewportRect.Height() >> HiResMipLevel, 1));
			const FIntPoint HiResDispatchThreadIDOffset(ViewportRect.Min.X >> HiResMipLevel, ViewportRect.Min.Y >> HiResMipLevel);

			{
				// upsample
				FVARIDBasicResampleCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDBasicResampleCS::FParameters>();
				PassParameters->InDispatchThreadIDOffset = HiResDispatchThreadIDOffset;
				PassParameters->InTexelSize = HiResTexelSize;
				PassParameters->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(OutContrastTexture, LoResMipLevel));
				PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(UpsampledMipTexture, HiResMipLevel));

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Build Contrast Texture - Upsample - MipLevel=%d", HiResMipLevel),
					ResampleComputeShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(HiResDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}

			{
				// blur
				FVARIDGaussianBlurCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDGaussianBlurCS::FParameters>();
				PassParameters->DispatchThreadIDOffset = HiResDispatchThreadIDOffset;
				PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(UpsampledMipTexture, HiResMipLevel));
				PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(BlurredMipTexture, HiResMipLevel));

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Build Contrast Texture - Gaussian Blur - MipLevel=%d", HiResMipLevel),
					GaussianBlurComputeShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(HiResDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}

			{
				// combine laplace and gaussian to reconstruct original image
				FVARIDReconstructCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDReconstructCS::FParameters>();
				PassParameters->InDispatchThreadIDOffset = HiResDispatchThreadIDOffset;
				PassParameters->InGaussianSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(BlurredMipTexture, HiResMipLevel));
				PassParameters->InLaplacianSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InLaplacianMipTexture, HiResMipLevel));
				PassParameters->InVFMapSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InVFMapMipTexture, HiResMipLevel));
				PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutContrastTexture, HiResMipLevel));

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Build Contrast Texture - Reconstruct - MipLevel=%d", HiResMipLevel),
					ReconstructComputeShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(HiResDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}
		}
	}
}

static bool BuildInpaintTexturePIXMIX_RenderThread(FRDGBuilder& GraphBuilder, FRDGTextureRef InOriginalTexture, FRDGTextureRef InVFMapTexture, FRDGTextureRef OutPositionMipTexture, const FIntRect& ViewportRect)
{
	check(InOriginalTexture);
	check(InVFMapTexture);
	check(OutPositionMipTexture);

	// VF map when passed in only has mip level: zero - but it can take more levels. Populate them here. 
	// We use a point sampler as we dont want any interpolation of the mask values. should be either black (0) or white (1)
	// TODO remove? ahhh but then wouldnt be able to do pixel id addressing - would have to use sampleLevel()
	// TODO only generate levels we actually use - so not down to 1x1...
	if (!BuildPyramid_RenderThread(GraphBuilder, InVFMapTexture, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), ViewportRect))
	{
		return false;
	}

	const FRDGTextureDesc& OutTextureDesc = OutPositionMipTexture->Desc;
	const int32 OriginalTextureWidth = OutTextureDesc.Extent.X;
	const int32 OriginalTextureHeight = OutTextureDesc.Extent.Y;
	const int32 OriginalTextureOriginOffset = ViewportRect.Min.X > 0 ? OriginalTextureWidth / 2 : 0;
	const int32 MaxNumberOfCostReducingIterations = 1;
	const int32 NumberOfMips = 6;// OutTextureDesc.NumMips;
	const int32 MaxMipLevelIndex = NumberOfMips - 1;	// offset for zero based index

	FRDGTextureDesc PositionCostMipTextureDesc = FRDGTextureDesc::Create2D
	(
		OutTextureDesc.Extent,
		EPixelFormat::PF_A32B32G32R32F,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_UAV,
		NumberOfMips,
		1
	);

	FRDGTextureRef PositionCostMipTexture = GraphBuilder.CreateTexture(PositionCostMipTextureDesc, TEXT("PositionCostMipTexture"));

	// all shaders
	TShaderMapRef<FVARIDInpainterRandomPositionCS> RandomPositionMapShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDInpainterCostCS> CostShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDInpainterPropagateCS> PropagateShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDBasicResampleCS> ResampleComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	// at the coarsest mip level, init masked area with random positions and unmasked areas with linear UV values
	{
		const FIntPoint InitTextureSize(FMath::Max(OriginalTextureWidth >> MaxMipLevelIndex, 1), FMath::Max(OriginalTextureHeight >> MaxMipLevelIndex, 1));
		const FVector2D InitTexelSize(1.0f / InitTextureSize.X, 1.0f / InitTextureSize.Y);
		const FIntPoint InitDispatchSize(FMath::Max(OriginalTextureWidth >> MaxMipLevelIndex, 1), FMath::Max(OriginalTextureHeight >> MaxMipLevelIndex, 1));
		const FIntPoint InitDispatchThreadIDOffset(OriginalTextureOriginOffset >> MaxMipLevelIndex, 0);

		{
			FVARIDInpainterRandomPositionCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDInpainterRandomPositionCS::FParameters>();
			PassParameters->InDispatchThreadIDOffset = InitDispatchThreadIDOffset;
			PassParameters->InTexelSize = InitTexelSize;
			PassParameters->InBilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParameters->InPointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParameters->InMaskSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InVFMapTexture, MaxMipLevelIndex));
			PassParameters->OutPositionUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutPositionMipTexture, MaxMipLevelIndex));
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Inpainter - Generate Random Positions - MipLevel=%d", MaxMipLevelIndex),
				RandomPositionMapShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(InitDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
		}
	}

	// work from the highest mip level (lowest resolution) to the lowest mip level (highest resolution)
	for (int32 MipLevel = MaxMipLevelIndex; MipLevel >= 0; --MipLevel) // dont use unsigned int here - otherwise numbers will overflow to a large positive number if it tries to subtract from zero...
	{
		const FIntPoint TextureSize(FMath::Max(OriginalTextureWidth >> MipLevel, 1), FMath::Max(OriginalTextureHeight >> MipLevel, 1));
		const FVector2D TexelSize(1.0f / TextureSize.X, 1.0f / TextureSize.Y);
		const FIntPoint DispatchSize(FMath::Max(OriginalTextureWidth >> MipLevel, 1), FMath::Max(OriginalTextureHeight >> MipLevel, 1));
		const FIntPoint DispatchThreadIDOffset(OriginalTextureOriginOffset >> MipLevel, 0);

		// refine 
		for (int32 CostReducingIteration = 0; CostReducingIteration < MaxNumberOfCostReducingIterations; ++CostReducingIteration)
		{
			// NOTE: during first iteration the masked area of the position map will have completely random values and no cost data 

			// steps:
			// 1. calculate current cost - first iteration this will be comparing the initial random values 
			//		inputs: position, cost
			//		outputs: position + cost
			// 2. propagate neighbour mappings with lowest costs
			//		each pixel compares itself with its neighbours w.r.t. cost and copies mapping with the lowest cost (which could already be itself therefore no change)
			//		inputs: cost
			//		outputs: position

			{
				FVARIDInpainterCostCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDInpainterCostCS::FParameters>();
				PassParameters->InDispatchThreadIDOffset = DispatchThreadIDOffset;
				PassParameters->InTexelSize = TexelSize;
				PassParameters->InBilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParameters->InPointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParameters->InTextureSize = TextureSize;
				PassParameters->InFloatMax = FLT_MAX;
				PassParameters->InColourSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InOriginalTexture, MipLevel));	// colour used for appearance cost calculation. 
				PassParameters->InMaskSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InVFMapTexture, MipLevel));
				PassParameters->InPositionSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(OutPositionMipTexture, MipLevel));
				PassParameters->OutPositionCostUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(PositionCostMipTexture, MipLevel));

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Inpainter - Calc Position Cost - MipLevel=%d - CostReducingIteration=%d", MipLevel, CostReducingIteration),
					CostShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}

			{
				FVARIDInpainterPropagateCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDInpainterPropagateCS::FParameters>();
				PassParameters->InDispatchThreadIDOffset = DispatchThreadIDOffset;
				PassParameters->InTexelSize = TexelSize;
				PassParameters->InBilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParameters->InPointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParameters->InTextureSize = TextureSize;
				PassParameters->InColourSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InOriginalTexture, MipLevel));	// colour used for appearance cost calculation. 
				PassParameters->InMaskSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InVFMapTexture, MipLevel));
				PassParameters->InPositionCostSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(PositionCostMipTexture, MipLevel));
				PassParameters->OutPositionUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutPositionMipTexture, MipLevel));
				
				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Inpainter - Propagate Low cost Positions - MipLevel=%d - CostReducingIteration=%d", MipLevel, CostReducingIteration),
					PropagateShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}
		}

		// upsample
		if (MipLevel > 0)
		{
			int32 LoResMipLevel = MipLevel;
			int32 HiResMipLevel = MipLevel - 1;

			const FIntPoint UpsampledTextureSize(FMath::Max(OriginalTextureWidth >> HiResMipLevel, 1), FMath::Max(OriginalTextureHeight >> HiResMipLevel, 1));
			const FVector2D UpsampledTexelSize(1.0f / UpsampledTextureSize.X, 1.0f / UpsampledTextureSize.Y);
			const FIntPoint UpsampledDispatchSize(FMath::Max(OriginalTextureWidth >> HiResMipLevel, 1), FMath::Max(OriginalTextureHeight >> HiResMipLevel, 1));
			const FIntPoint UpsampledDispatchThreadIDOffset(OriginalTextureOriginOffset >> HiResMipLevel, 0);

			{
				FVARIDBasicResampleCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDBasicResampleCS::FParameters>();
				PassParameters->InDispatchThreadIDOffset = UpsampledDispatchThreadIDOffset;
				PassParameters->InTexelSize = UpsampledTexelSize;
				PassParameters->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParameters->InSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(OutPositionMipTexture, LoResMipLevel));
				PassParameters->OutUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutPositionMipTexture, HiResMipLevel));

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("VARID - Inpainter - Position Upsample - MipLevel=%d", MipLevel),
					ResampleComputeShader,
					PassParameters,
					FComputeShaderUtils::GetGroupCount(UpsampledDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
			}
		}
	}

	return true;
}

static bool BuildInpaintTexture_RenderThread(FRDGBuilder& GraphBuilder, FRDGTextureRef InOriginalTexture, FRDGTextureRef InVFMapTexture, FRDGTextureRef OutPositionMipTexture, const FIntRect& ViewportRect)
{
	check(InOriginalTexture);
	check(InVFMapTexture);
	check(OutPositionMipTexture);

	// VF map when passed in only has mip level: zero - but it can take more levels. Populate them here. 
	// We use a point sampler as we dont want any interpolation of the mask values. should be either black (0) or white (1)
	// TODO remove? ahhh but then wouldnt be able to do pixel id addressing - would have to use sampleLevel()
	// TODO only generate level we actually use - so not down to 1x1...
	if (!BuildPyramid_RenderThread(GraphBuilder, InVFMapTexture, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), ViewportRect))
	{
		return false;
	}

	const FRDGTextureDesc& OutTextureDesc = OutPositionMipTexture->Desc;
	const int32 OriginalTextureWidth = OutTextureDesc.Extent.X;
	const int32 OriginalTextureHeight = OutTextureDesc.Extent.Y;
	const int32 OriginalTextureOriginOffset = ViewportRect.Min.X > 0 ? OriginalTextureWidth / 2 : 0;
	const FVector2D OriginalTexelSize(1.0f / OriginalTextureWidth, 1.0f / OriginalTextureHeight);
	const FIntPoint OriginalDispatchSize(OriginalTextureWidth, OriginalTextureHeight);
	const FIntPoint OriginalDispatchThreadIDOffset(OriginalTextureOriginOffset, 0);

	const int32 NumberOfPasses = 20;
	const int32 MetaDataMipLevel = 4;

	// change 'inpainted mask' to 'meta data'

	FRDGTextureDesc MetaDataTextureDesc = FRDGTextureDesc::Create2D
	(
		OutTextureDesc.Extent,
		EPixelFormat::PF_A32B32G32R32F,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_UAV,
		OutTextureDesc.NumMips,
		1
	);

	FRDGTextureRef MetaDataTexture_1 = GraphBuilder.CreateTexture(MetaDataTextureDesc, TEXT("MetaDataTexture_1"));
	FRDGTextureRef MetaDataTexture_2 = GraphBuilder.CreateTexture(MetaDataTextureDesc, TEXT("MetaDataTexture_2"));
	FRDGTextureRef ColourTexture_1 = GraphBuilder.CreateTexture(InOriginalTexture->Desc, TEXT("ColourTexture_1"));
	FRDGTextureRef ColourTexture_2 = GraphBuilder.CreateTexture(InOriginalTexture->Desc, TEXT("ColourTexture_2"));

	TShaderMapRef<FVARIDInpainterInitialiseCS> InpainterInitialiseShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDInpainterFillCS> InpainterShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FVARIDInpainterFinaliseCS> InpainterFinaliseShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	const FIntPoint TextureSize(FMath::Max(OriginalTextureWidth >> MetaDataMipLevel, 1), FMath::Max(OriginalTextureHeight >> MetaDataMipLevel, 1));
	const FVector2D TexelSize(1.0f / TextureSize.X, 1.0f / TextureSize.Y);
	const FIntPoint DispatchSize(FMath::Max(OriginalTextureWidth >> MetaDataMipLevel, 1), FMath::Max(OriginalTextureHeight >> MetaDataMipLevel, 1));
	const FIntPoint DispatchThreadIDOffset(OriginalTextureOriginOffset >> MetaDataMipLevel, 0);

	// initialise
	{
		FVARIDInpainterInitialiseCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDInpainterInitialiseCS::FParameters>();
		PassParameters->InDispatchThreadIDOffset = DispatchThreadIDOffset;
		PassParameters->InTexelSize = TexelSize;
		PassParameters->InMaskSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InVFMapTexture, MetaDataMipLevel));
		PassParameters->OutMetaDataUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(MetaDataTexture_1, MetaDataMipLevel));
		
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("VARID - Inpainter - Initialise - MipLevel=%d", MetaDataMipLevel),
			InpainterInitialiseShader,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
	}

	// pointers to textures
	FRDGTextureRef InMetaData;
	FRDGTextureRef OutMetaData;
	FRDGTextureRef InColour;
	FRDGTextureRef OutColour;

	// refine 
	for (int32 PassCounter = 0; PassCounter < NumberOfPasses; ++PassCounter)
	{
		// flipping totally works!
		if (PassCounter % 2 == 0)
		{
			InMetaData = MetaDataTexture_1;
			OutMetaData = MetaDataTexture_2;
			InColour = ColourTexture_1;
			OutColour = ColourTexture_2;
		}
		else //if (CostReducingIteration % 2 == 1)
		{
			InMetaData = MetaDataTexture_2;
			OutMetaData = MetaDataTexture_1;
			InColour = ColourTexture_2;
			OutColour = ColourTexture_1;
		}

		{
			FVARIDInpainterFillCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDInpainterFillCS::FParameters>();
			PassParameters->InDispatchThreadIDOffset = DispatchThreadIDOffset;
			PassParameters->InTexelSize = TexelSize;
			PassParameters->PassCounter = PassCounter;
			PassParameters->InColourSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InColour, MetaDataMipLevel));
			PassParameters->InMaskSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InVFMapTexture, MetaDataMipLevel));
			PassParameters->InMetaDataSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InMetaData, MetaDataMipLevel));
			PassParameters->OutMetaDataUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutMetaData, MetaDataMipLevel));
			PassParameters->OutColourUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutColour, MetaDataMipLevel));

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VARID - Inpainter - MipLevel=%d - PassCounter=%d", MetaDataMipLevel, PassCounter),
				InpainterShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(DispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
		}
	}

	// finalise
	{
		FVARIDInpainterFinaliseCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDInpainterFinaliseCS::FParameters>();
		PassParameters->InDispatchThreadIDOffset = OriginalDispatchThreadIDOffset;
		PassParameters->InTexelSize = OriginalTexelSize;
		PassParameters->InBilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->InPointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->InColourSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InOriginalTexture, 0));
		PassParameters->InMaskSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(InVFMapTexture, 0));
		PassParameters->InMetaDataSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(OutMetaData, MetaDataMipLevel));
		PassParameters->OutPositionUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutPositionMipTexture, 0));

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("VARID - Inpainter - Finalise - MipLevel=%d", 0),
			InpainterFinaliseShader,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(OriginalDispatchSize, FComputeShaderUtils::kGolden2DGroupSize));
	}

	return true;
}


/*****************************************************************************************************************/
// helpers

static uint8 CalculateNumMips1D(int32 InValue)
{
	uint8 numTimesHalved = 0;
	while (InValue > 1)
	{
		// could also be done by logarithm to base 2 rounded up/down to the next integer
		InValue = InValue >> 1;
		numTimesHalved++;
	}

	return numTimesHalved;
}

static uint8 CalculateNumMips2D(FIntPoint Size)
{
	uint8 mipsWidth = CalculateNumMips1D(Size.X);
	uint8 mipsHeight = CalculateNumMips1D(Size.Y);
	return mipsWidth > mipsHeight ? mipsWidth : mipsHeight;
}


/*****************************************************************************************************************/
// scene view extenstion

FVARIDSceneViewExtension::FVARIDSceneViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{

}

void FVARIDSceneViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	// this method runs in the game thread before the VARID rendering is performed
	// It is here that we marshall the data from the game thread to the render thread. 

	FVARIDProfile& Profile = FVARIDModule::Get().GetActiveProfile();
	FVARIDEyeTracking& EyeTracking = FVARIDModule::Get().GetEyeTracking();

	// TODO prevent copy constructor being called twice for each parameter. try converting FCachedRenderResource to hold pointers. 

	// NOTE: this calls copy constructor for each parameter so we dont have to do it explicity. 
	ENQUEUE_RENDER_COMMAND(VARIDParameters)(
		[
			this,
			Profile,
			EyeTracking
		](FRHICommandListImmediate& RHICmdList)
		{
			// these assignments using equals operate actually results in 'Copy Initialization' - the copy constructor is called
			CachedResourcesRenderThread.Profile = Profile;
			CachedResourcesRenderThread.EyeTracking = EyeTracking;
		}
	);
}

void FVARIDSceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass PassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	// EPostProcessingPass:
	//   MotionBlur
	//   Tonemap
	//   FXAA
	//   VisualizeDepthOfField

	if (PassId == EPostProcessingPass::Tonemap)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FVARIDSceneViewExtension::PostProcessPassAfterTonemap_RenderThread));
	}
}

FScreenPassTexture FVARIDSceneViewExtension::PostProcessPassAfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutMaterialInputs)
{
	// NOTE: SceneColour format is B8G8R8A8_UNORM

	check(IsInRenderingThread());

	const FScreenPassTexture& SceneColor = InOutMaterialInputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor];
	const FIntPoint TextureSize = SceneColor.Texture->Desc.Extent;
	const FIntRect ViewportRect = SceneColor.ViewRect;

	// NOTE! 
	// scene colour texture is reused for left and right rendering
	// if stereo, the image that is passed in already includes lens mesh distortion and masking
	// the texture has to be full size i.e. to cover both eyes for stereo
	// the view port covers just one eye
	// view 0 = left = draws to the left side of the texture
	// view 1 = right = draws to the right side of the same texture
	// if stereo, pixel shader takes full size texture but only draws to one half of the texture

	if (!(SceneColor.Texture->Desc.Flags & ETextureCreateFlags::TexCreate_ShaderResource))
	{
		return SceneColor;
	}

	if (!SceneColor.IsValid())
	{
		return SceneColor;
	}

	if (!CachedResourcesRenderThread.Profile.IsValid)
	{
		return SceneColor;
	}

	RDG_EVENT_SCOPE(GraphBuilder, "VARID Rendering");
	{

		/*************************************************************/
		// setup back buffer to render to

		FScreenPassRenderTarget BackBufferRenderTarget;

		// If the override output is provided it means that this is the last pass in post processing. JJB huh? TODO revise. this explanation originally comes from the OpenColor scene view extension.
		if (InOutMaterialInputs.OverrideOutput.IsValid())
		{
			// VR
			// override
			BackBufferRenderTarget = InOutMaterialInputs.OverrideOutput;
		}
		else
		{
			// PIE - non VR - desktop
			// NOT overridden

			// Reusing the same output description for our back buffer as SceneColor when it's not overridden
			FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;
			OutputDesc.Extent = TextureSize;
			OutputDesc.Flags |= TexCreate_RenderTargetable;
			FLinearColor ClearColor(0., 0., 0., 0.);
			OutputDesc.ClearValue = FClearValueBinding(ClearColor);

			FRDGTexture* BackBufferRenderTargetTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("BackBufferRenderTargetTexture"));
			BackBufferRenderTarget = FScreenPassRenderTarget(BackBufferRenderTargetTexture, ViewportRect, ERenderTargetLoadAction::EClear);
		}

		uint8 NumberOfMipsToGenerate = CalculateNumMips2D(TextureSize);

		NumberOfMipsToGenerate = FMath::Min(NumberOfMipsToGenerate, MAX_NUM_MIP_LEVELS);


		/*************************************************************/

		FRDGTextureDesc R32_FLOAT_TextureDesc = FRDGTextureDesc::Create2D
		(
			TextureSize,
			EPixelFormat::PF_R32_FLOAT,		// TODO recuce to 16?
			FClearValueBinding::Black,
			TexCreate_ShaderResource | TexCreate_UAV,	// TODO revise
			NumberOfMipsToGenerate,
			1
		);

		FRDGTextureDesc G32R32F_TextureDesc = FRDGTextureDesc::Create2D
		(
			TextureSize,
			EPixelFormat::PF_G32R32F,		// TODO reduce to 16?
			FClearValueBinding::Black,
			TexCreate_ShaderResource | TexCreate_UAV,
			NumberOfMipsToGenerate,
			1
		);

		FRDGTextureDesc A32B32G32R32F_TextureDesc = FRDGTextureDesc::Create2D
		(
			TextureSize,
			EPixelFormat::PF_A32B32G32R32F,
			FClearValueBinding::Black,
			TexCreate_ShaderResource | TexCreate_UAV,
			NumberOfMipsToGenerate,
			1
		);

		FRDGTextureDesc R16G16B16A16_UNORM_TextureDesc = FRDGTextureDesc::Create2D
		(
			TextureSize,
			EPixelFormat::PF_R16G16B16A16_UNORM,	// original is 8 bits per channel... we give it double precision to work with compared to input and output format
			FClearValueBinding::Green,
			TexCreate_ShaderResource | TexCreate_UAV,	// TODO revise
			NumberOfMipsToGenerate,
			1
		);

		/*************************************************************/
		// build VF maps and vertex buffers

		// NOTE: some VF maps e.g. contrast and inpaint require a mip map texture, so all VF map textures are created with the ability to be a mip map

		FRDGTextureRef BlurVFMapTexture = GraphBuilder.CreateTexture(R32_FLOAT_TextureDesc, TEXT("BlurVFMapTexture"));
		FRDGTextureRef ContrastVFMapTexture = GraphBuilder.CreateTexture(R32_FLOAT_TextureDesc, TEXT("ContrastVFMapTexture"));
		FRDGTextureRef InpaintVFMapTexture = GraphBuilder.CreateTexture(R32_FLOAT_TextureDesc, TEXT("InpaintVFMapTexture"));
		FRDGTextureRef WarpVFMapTexture = GraphBuilder.CreateTexture(G32R32F_TextureDesc, TEXT("WarpVFMapTexture"));

		FRHIVertexBuffer* VertexBuffer = nullptr;

		// do any eye specific code here
		switch (View.StereoPass)
		{
		case eSSP_FULL:
			VertexBuffer = GQuadVertexBufferFull.VertexBufferRHI;
			BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.LeftEye.Blur.Enabled, CachedResourcesRenderThread.Profile.LeftEye.Blur.VFMap.Data, 0, CachedResourcesRenderThread.EyeTracking.LeftEyeGazePoint, 0.0f, BlurVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.LeftEye.Blur.VFMap.FullField);
			for (int32 MipLevel = 0; MipLevel < NumberOfMipsToGenerate; MipLevel++)
			{
				BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.LeftEye.Contrast.Enabled, CachedResourcesRenderThread.Profile.LeftEye.Contrast.VFMaps[MipLevel].Data, MipLevel, CachedResourcesRenderThread.EyeTracking.LeftEyeGazePoint, 0.0f, ContrastVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.LeftEye.Contrast.VFMaps[MipLevel].FullField);
			}
			BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.LeftEye.Inpaint.Enabled, CachedResourcesRenderThread.Profile.LeftEye.Inpaint.VFMap.Data, 0, CachedResourcesRenderThread.EyeTracking.LeftEyeGazePoint, 0.0f, InpaintVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.LeftEye.Inpaint.VFMap.FullField);
			BuildNormalMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.LeftEye.Warp.Enabled, CachedResourcesRenderThread.Profile.LeftEye.Warp.VFMap.Data, CachedResourcesRenderThread.EyeTracking.LeftEyeGazePoint, 0.5f, WarpVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.LeftEye.Warp.VFMap.FullField);
			break;
		case eSSP_LEFT_EYE:
			VertexBuffer = GQuadVertexBufferLeft.VertexBufferRHI;
			BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.LeftEye.Blur.Enabled, CachedResourcesRenderThread.Profile.LeftEye.Blur.VFMap.Data, 0, CachedResourcesRenderThread.EyeTracking.LeftEyeGazePoint, 0.0f, BlurVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.LeftEye.Blur.VFMap.FullField);
			for (int32 MipLevel = 0; MipLevel < NumberOfMipsToGenerate; MipLevel++)
			{
				BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.LeftEye.Contrast.Enabled, CachedResourcesRenderThread.Profile.LeftEye.Contrast.VFMaps[MipLevel].Data, MipLevel, CachedResourcesRenderThread.EyeTracking.LeftEyeGazePoint, 0.0f, ContrastVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.LeftEye.Contrast.VFMaps[MipLevel].FullField);
			}
			BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.LeftEye.Inpaint.Enabled, CachedResourcesRenderThread.Profile.LeftEye.Inpaint.VFMap.Data, 0, CachedResourcesRenderThread.EyeTracking.LeftEyeGazePoint, 0.0f, InpaintVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.LeftEye.Inpaint.VFMap.FullField);
			BuildNormalMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.LeftEye.Warp.Enabled, CachedResourcesRenderThread.Profile.LeftEye.Warp.VFMap.Data, CachedResourcesRenderThread.EyeTracking.LeftEyeGazePoint, 0.5f, WarpVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.LeftEye.Warp.VFMap.FullField);
			break;
		case eSSP_RIGHT_EYE:
			VertexBuffer = GQuadVertexBufferRight.VertexBufferRHI;
			BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.RightEye.Blur.Enabled, CachedResourcesRenderThread.Profile.RightEye.Blur.VFMap.Data, 0, CachedResourcesRenderThread.EyeTracking.RightEyeGazePoint, 0.0f, BlurVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.RightEye.Blur.VFMap.FullField);
			for (int32 MipLevel = 0; MipLevel < NumberOfMipsToGenerate; MipLevel++)
			{
				BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.RightEye.Contrast.Enabled, CachedResourcesRenderThread.Profile.RightEye.Contrast.VFMaps[MipLevel].Data, MipLevel, CachedResourcesRenderThread.EyeTracking.RightEyeGazePoint, 0.0f, ContrastVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.RightEye.Contrast.VFMaps[MipLevel].FullField);
			}
			BuildHeightMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.RightEye.Inpaint.Enabled, CachedResourcesRenderThread.Profile.RightEye.Inpaint.VFMap.Data, 0, CachedResourcesRenderThread.EyeTracking.RightEyeGazePoint, 0.0f, InpaintVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.RightEye.Inpaint.VFMap.FullField);
			BuildNormalMapTexture_RenderThread(GraphBuilder, CachedResourcesRenderThread.Profile.RightEye.Warp.Enabled, CachedResourcesRenderThread.Profile.RightEye.Warp.VFMap.Data, CachedResourcesRenderThread.EyeTracking.RightEyeGazePoint, 0.5f, WarpVFMapTexture, ViewportRect, View.StereoPass, CachedResourcesRenderThread.Profile.RightEye.Warp.VFMap.FullField);
			break;
		default:
			break;
		}

		/*************************************************************/
		// build FX


		FRDGTextureRef GaussianTexture = GraphBuilder.CreateTexture(R16G16B16A16_UNORM_TextureDesc, TEXT("GaussianTexture"));
		BuildGaussianPyramid_RenderThread(GraphBuilder, SceneColor.Texture, GaussianTexture, ViewportRect);

		FRDGTextureRef LaplacianTexture = GraphBuilder.CreateTexture(R16G16B16A16_UNORM_TextureDesc, TEXT("LaplacianTexture"));
		BuildLaplacianPyramid_RenderThread(GraphBuilder, GaussianTexture, LaplacianTexture, ViewportRect);

		FRDGTextureRef ContrastTexture = GraphBuilder.CreateTexture(R16G16B16A16_UNORM_TextureDesc, TEXT("ContrastTexture"));
		BuildContrastTexture_RenderThread(GraphBuilder, LaplacianTexture, ContrastVFMapTexture, ContrastTexture, ViewportRect);

		FRDGTextureRef InpaintPositionTexture = GraphBuilder.CreateTexture(G32R32F_TextureDesc, TEXT("InpaintPositionTexture"));
		BuildInpaintTexture_RenderThread(GraphBuilder, ContrastTexture, InpaintVFMapTexture, InpaintPositionTexture, ViewportRect);	//outputs a position texture

		/*************************************************************/

		{
			TShaderMapRef<FVARIDQuadVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			TShaderMapRef<FVARIDQuadPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			FVARIDQuadPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVARIDQuadPS::FParameters>();
			PassParameters->InTrilinearSampler = TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParameters->InBilinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParameters->InPointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParameters->InMaxMipLevel = NumberOfMipsToGenerate;

			PassParameters->InGaussianSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(GaussianTexture));
			PassParameters->InLaplacianSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(LaplacianTexture));
			PassParameters->InContrastSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(ContrastTexture));
			PassParameters->InInpaintSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InpaintPositionTexture));

			PassParameters->InBlurVFMapSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(BlurVFMapTexture));
			PassParameters->InContrastVFMapSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(ContrastVFMapTexture));
			PassParameters->InInpaintVFMapSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InpaintVFMapTexture));
			PassParameters->InWarpVFMapSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(WarpVFMapTexture));

			PassParameters->RenderTargets[0] = BackBufferRenderTarget.GetRenderTargetBinding();

			ClearUnusedGraphResources(PixelShader, PassParameters);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("VARID FX Compositor"),
				PassParameters,
				ERDGPassFlags::Raster,
				[
					&View,
					VertexBuffer,
					VertexShader,
					PixelShader,
					ViewportRect,
					PassParameters
				](FRHICommandListImmediate& RHICmdList)
				{
					FGraphicsPipelineStateInitializer GraphicsPSOInit;
					RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
					GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
					GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
					GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
					GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
					SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);
					RHICmdList.SetStreamSource(0, VertexBuffer, 0);
					RHICmdList.SetViewport(ViewportRect.Min.X, ViewportRect.Min.Y, 0.0f, ViewportRect.Max.X, ViewportRect.Max.Y, 1.0f);
					RHICmdList.DrawPrimitive(0, 2, 1);
				});	// end pass

			return MoveTemp(BackBufferRenderTarget);
		}

	} //end RDG scope
}