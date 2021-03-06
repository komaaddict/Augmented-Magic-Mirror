#pragma once

class Camera;

namespace D3DX12
{
	class GraphicsContext;

	class RenderingContext
	{
	public:
		RenderingContext(_In_ GraphicsContext & DeviceContext);

		void Create();
		void Prepare(_In_ const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> & CommandList, _In_ const Camera & Camera) const;
		void SetObjectMatrix(_In_ const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> & CommandList, _In_ const DirectX::XMFLOAT4X4 & ObjectMatrix) const;

	private:
		static constexpr UINT Num32BitPerMatrix = 4 * 4;

		GraphicsContext & DeviceContext;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;

		void CreateRootSignature();
		void CreatePipelineState();
	};
}