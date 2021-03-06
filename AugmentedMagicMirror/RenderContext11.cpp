// RenderContext.cpp : A context for rendering to a particular output Window
//

#include "stdafx.h"
#ifdef USE_D3DX11
#include "RenderContext11.h"

#include "GraphicsContext11.h"
#include "Mesh11.h"

#include "Camera.h"

namespace D3DX11
{
	RenderContext::RenderContext(_In_ GraphicsContext & DeviceContext, _In_ Window & TargetWindow, _In_ Camera & NoseCamera, _In_ Camera & LeftEyeCamera, _In_ Camera & RighEyeCamera)
		: ::RenderContext(TargetWindow, NoseCamera, LeftEyeCamera, RighEyeCamera)
		, DeviceContext(DeviceContext)
		, StereoEnabled(false), ForceMono(false)
		, Viewport({}), ScissorRect({})
	{
	}

	void RenderContext::Initialize()
	{
		if (TargetWindow.GetHandle() == nullptr)
		{
			Utility::Throw(L"Window Handle is invalid!");
		}

		DeviceContext.StereoStatusChanged += std::make_pair(this, &RenderContext::OnStereoStatusChanged);
		TargetWindow.WindowResized += std::make_pair(this, &RenderContext::OnWindowSizeChange);
		TargetWindow.KeyPressed += std::make_pair(this, &RenderContext::KeyPressedCallback);

		UpdateStereoStatus();
		CreateSizeDependantResources();
	}

	void RenderContext::Render(_In_ MeshList DrawCalls)
	{
		if (StereoEnabled)
		{
			RenderStereo(DrawCalls);
		}
		else
		{
			RenderEye(DrawCalls, NoseCamera, RTVLeft);
		}

		SwapChain->Present(0, 0);
	}

	void RenderContext::RenderStereo(_In_ MeshList DrawCalls)
	{
		RenderEye(DrawCalls, LeftEyeCamera, RTVLeft);
		RenderEye(DrawCalls, ForceMono ? LeftEyeCamera : RighEyeCamera, RTVRight);
	}

	void RenderContext::RenderEye(_In_ MeshList DrawCalls, _In_ const Camera & View, _In_ Microsoft::WRL::ComPtr<ID3D11RenderTargetView> & RTV)
	{
		std::array<ID3D11RenderTargetView *const, 1> RTVs = { RTV.Get() };
		DeviceContext.GetDeviceContext()->OMSetRenderTargets(1, RTVs.data(), DSV.Get());
		DeviceContext.GetDeviceContext()->ClearRenderTargetView(RTVs[0], BackgroundColor.data());
		DeviceContext.GetDeviceContext()->ClearDepthStencilView(DSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		DeviceContext.GetDeviceContext()->RSSetViewports(1, &Viewport);
		DeviceContext.GetDeviceContext()->RSSetScissorRects(1, &ScissorRect);

		DeviceContext.GetDefaultShader().Prepare(View);

		for (ObjectList & ObjectsToRender : DrawCalls)
		{
			const Mesh & Mesh11 = static_cast<const Mesh &>(ObjectsToRender.first);
			Mesh11.Render(DeviceContext.GetDefaultShader(), ObjectsToRender.second);
		}
	}

	void RenderContext::OnStereoStatusChanged()
	{
		ReleaseSizeDependantResources(true);
		UpdateStereoStatus();
		CreateSizeDependantResources();
	}

	void RenderContext::OnWindowSizeChange(_In_ const Window::WindowSize & NewSize)
	{
		if (!SwapChain)
			return;

		ReleaseSizeDependantResources(false);

		Utility::ThrowOnFail(SwapChain->ResizeBuffers(BufferFrameCount, NewSize.first, NewSize.second, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
		
		CreateSizeDependantResources(NewSize);
	}

	void RenderContext::KeyPressedCallback(const WPARAM & VirtualKey)
	{
		if (VirtualKey == 'G' && StereoEnabled)
		{
			ForceMono = !ForceMono;
		}
	}

	void RenderContext::CreateSizeDependantResources()
	{
		const Window::WindowSize & Size = TargetWindow.GetWindowSize();
		CreateSizeDependantResources(Size);
	}

	void RenderContext::CreateSizeDependantResources(_In_ const Window::WindowSize & Size)
	{
		if (!SwapChain)
		{
			CreateSwapChain(Size);
		}

		CreateRenderTargets();
		CreateDepthStencil(Size);

		UpdateCameras(Size);
		UpdateViewportAndScissorRect(Size);
	}

	void RenderContext::ReleaseSizeDependantResources(bool ReleaseSwapChain)
	{
		RTVLeft.Reset();
		RTVRight.Reset();
		DSV.Reset();

		if (ReleaseSwapChain)
		{
			SwapChain->SetFullscreenState(FALSE, nullptr);
			SwapChain.Reset();
		}
	}

	void RenderContext::UpdateStereoStatus()
	{
		StereoEnabled = DeviceContext.IsStereoEnabled();
	}

	void RenderContext::CreateSwapChain(_In_ const Window::WindowSize & Size)
	{
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.BufferCount = BufferFrameCount;
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.Stereo = StereoEnabled;

		SwapChainDesc.Width = Size.first;
		SwapChainDesc.Height = Size.second;

		Utility::ThrowOnFail(DeviceContext.GetFactory()->CreateSwapChainForHwnd(DeviceContext.GetDevice().Get(), TargetWindow.GetHandle(), &SwapChainDesc, nullptr, nullptr, &SwapChain));
	}

	void RenderContext::CreateRenderTargets()
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBuffer;

		SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
		RTVDesc.Format = DXGI_FORMAT_UNKNOWN;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.MipSlice = 0;
		RTVDesc.Texture2DArray.FirstArraySlice = 0;
		RTVDesc.Texture2DArray.ArraySize = 1;

		Utility::ThrowOnFail(DeviceContext.GetDevice()->CreateRenderTargetView(BackBuffer.Get(), &RTVDesc, &RTVLeft));

		if (StereoEnabled)
		{
			RTVDesc.Texture2DArray.FirstArraySlice = 1;

			Utility::ThrowOnFail(DeviceContext.GetDevice()->CreateRenderTargetView(BackBuffer.Get(), &RTVDesc, &RTVRight));
		}
	}

	void RenderContext::CreateDepthStencil(_In_ const Window::WindowSize & Size)
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> DepthStencilBuffer;

		D3D11_TEXTURE2D_DESC DepthDesc = {};
		DepthDesc.Width = Size.first;
		DepthDesc.Height = Size.second;
		DepthDesc.MipLevels = 1;
		DepthDesc.ArraySize = 2;
		DepthDesc.Format = DXGI_FORMAT_D32_FLOAT;
		DepthDesc.SampleDesc.Count = 1;
		DepthDesc.SampleDesc.Quality = 0;
		DepthDesc.Usage = D3D11_USAGE_DEFAULT;
		DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		DepthDesc.CPUAccessFlags = 0;
		DepthDesc.MiscFlags = 0;

		Utility::ThrowOnFail(DeviceContext.GetDevice()->CreateTexture2D(&DepthDesc, NULL, &DepthStencilBuffer));

		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format = DXGI_FORMAT_UNKNOWN;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Texture2D.MipSlice = 0;

		Utility::ThrowOnFail(DeviceContext.GetDevice()->CreateDepthStencilView(DepthStencilBuffer.Get(), &DSVDesc, &DSV)); 
	}

	void RenderContext::UpdateCameras(_In_ const Window::WindowSize & Size)
	{
		NoseCamera.UpdateCamera(Size);
		LeftEyeCamera.UpdateCamera(Size);
		RighEyeCamera.UpdateCamera(Size);
	}

	void RenderContext::UpdateViewportAndScissorRect(_In_ const Window::WindowSize & Size)
	{
		Viewport.Width = static_cast<float>(Size.first);
		Viewport.Height = static_cast<float>(Size.second);
		Viewport.MaxDepth = 1.0f;

		ScissorRect.right = static_cast<LONG>(Size.first);
		ScissorRect.bottom = static_cast<LONG>(Size.second);
	}
}
#endif
