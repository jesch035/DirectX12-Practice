#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	const int width = 800;
	const int height = 800;

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"DirectX12Triangle";
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(wc.lpszClassName, L"DirectX 12 Triangle", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);

	ShowWindow(hwnd, nCmdShow);

	// create device
	ID3D12Device* device = nullptr;
	HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

	// create command queue
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

	// create command allocator
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	hr = commandAllocator->Reset();

	// create command list
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	hr = commandList->Close();

	// create swap chain
	IDXGIFactory4* factory = nullptr;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	IDXGISwapChain1* tempSwapChain = nullptr;
	hr = factory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain);

	// cast the swap chain to IDXGISwapChain3
	IDXGISwapChain3* swapChain = nullptr;
	hr = tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain));
	tempSwapChain->Release();
	tempSwapChain = nullptr;

	// memory descriptor heap for render target views (RTV)
	ID3D12DescriptorHeap* rtvHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

	ID3D12Resource* renderTargets[2] = {};

	UINT rtcIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < 2; i++)
		{
			hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));

			device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
			rtvHandle.ptr += rtcIncrementSize;
		}
	}

	// create fence
	ID3D12Fence* fence = nullptr;
	UINT64 fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// create root signature
	D3D12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[0].Constants.Num32BitValues = 1;
	rootParameters[0].Constants.ShaderRegister = 0;
	rootParameters[0].Constants.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	ID3D12RootSignature* rootSignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	if (signatureBlob)
	{
		signatureBlob->Release();
		signatureBlob = nullptr;
	}

	if (errorBlob)
	{
		errorBlob->Release();
		errorBlob = nullptr;
	}

	// compile shaders
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;
	hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertexShader, nullptr);
	hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShader, nullptr);

	// pipeline state
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature;
	psoDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vertexShader->GetBufferSize();
	psoDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer();
	psoDesc.PS.BytecodeLength = pixelShader->GetBufferSize();

	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;

	D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {};
	defaultRenderTargetBlendDesc.BlendEnable = FALSE;
	defaultRenderTargetBlendDesc.LogicOpEnable = FALSE;
	defaultRenderTargetBlendDesc.SrcBlend = D3D12_BLEND_ONE;
	defaultRenderTargetBlendDesc.DestBlend = D3D12_BLEND_ZERO;
	defaultRenderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	defaultRenderTargetBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	defaultRenderTargetBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	defaultRenderTargetBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	defaultRenderTargetBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	defaultRenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;

	psoDesc.SampleMask = UINT_MAX;

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	psoDesc.InputLayout.pInputElementDescs = nullptr;
	psoDesc.InputLayout.NumElements = 0;
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	ID3D12PipelineState* pipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

	vertexShader->Release();
	vertexShader = nullptr;
	pixelShader->Release();
	pixelShader = nullptr;

	unsigned int triangleAngle = 0;

	bool running = true;
	while (running)
	{
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		hr = commandAllocator->Reset();
		hr = commandList->Reset(commandAllocator, nullptr);

		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = renderTargets[backBufferIndex];
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			commandList->ResourceBarrier(1, &barrier);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.ptr += backBufferIndex * rtcIncrementSize;

		// clear the render target
		float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// set viewport and scissor
		D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
		D3D12_RECT scissorRect = { 0, 0, LONG_MAX, LONG_MAX };
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootSignature(rootSignature);
		commandList->SetPipelineState(pipelineState);

		// draw the triangle
		commandList->SetGraphicsRoot32BitConstant(0, triangleAngle, 0);
		commandList->DrawInstanced(3, 1, 0, 0);

		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = renderTargets[backBufferIndex];
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			commandList->ResourceBarrier(1, &barrier);
		}

		hr = commandList->Close();

		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(1, commandLists);

		hr = swapChain->Present(1, 0);

		// wait on the CPU for the GPU frame to finish
		const UINT64 currentFenceValue = ++fenceValue;
		hr = commandQueue->Signal(fence, currentFenceValue);

		if (fence->GetCompletedValue() < currentFenceValue)
		{
			hr = fence->SetEventOnCompletion(currentFenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		triangleAngle++;
	}
}