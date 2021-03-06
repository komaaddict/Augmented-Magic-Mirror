#pragma once

#include "RenderContext.h"
#include "Mesh.h"

class Window;
class Camera;

class GraphicsContext;
typedef std::unique_ptr<GraphicsContext> PGraphicsContext;

PGraphicsContext CreateGraphicsContext();


class GraphicsContext
{
public:
	virtual ~GraphicsContext() = default; 

	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Release() = 0;

	virtual PRenderContext CreateRenderContext(_In_ Window & TargetWindow, _In_ Camera & NoseCamera, _In_ Camera & LeftEyeCamera, _In_ Camera & RighEyeCamera) = 0;
	virtual PMesh CreateMesh() = 0;

	static void LoadAndCompileShader(_Out_ Microsoft::WRL::ComPtr<ID3DBlob> & VertexShader, _Out_ Microsoft::WRL::ComPtr<ID3DBlob> & PixelShader, _In_ DWORD ShaderResourceId, _In_ const std::string & ShaderModel);
};

