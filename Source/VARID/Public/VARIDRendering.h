// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "VARIDProfile.h"
#include "VARIDEyeTracking.h"
#include "SceneViewExtension.h"

class FTextureResource;

class FVARIDSceneViewExtension : public FSceneViewExtensionBase
{
public:

	FVARIDSceneViewExtension(const FAutoRegister& AutoRegister);
		
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo) override {}
	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}
	virtual void PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override {}	
	virtual void PostRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override {}
	virtual void PostRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}
	virtual int32 GetPriority() const { return 0; }
	virtual bool IsActiveThisFrame(class FViewport* InViewport) const { return true; }
	virtual bool IsActiveThisFrameInContext(FSceneViewExtensionContext& Context) const { return IsActiveThisFrame(Context.Viewport); }

	// implemented
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;
	
	// VARID main render method
	FScreenPassTexture PostProcessPassAfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs);

private:

	struct FCachedRenderResource
	{		
		FVARIDProfile Profile;
		FVARIDEyeTracking EyeTracking;
	};

	// Local cached copy of the data. Purely used by render threads - hence privately defined within the main renderer class
	FCachedRenderResource CachedResourcesRenderThread;
};

