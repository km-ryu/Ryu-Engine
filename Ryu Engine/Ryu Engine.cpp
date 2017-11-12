#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <array>

using namespace std;

bool InitWnd(HINSTANCE, int);
bool InitDirectX();
void Render();
int Run();

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

const int CLIENT_WIDTH = 640;
const int CLIENT_HEIGHT = 480;

const D3D11_VIEWPORT BASIC_VIEWPORT{ 0.0f, 0.0f, CLIENT_WIDTH, CLIENT_HEIGHT, 0.0f, 1.0f };

HWND g_hMainWnd;

Microsoft::WRL::ComPtr<ID3D11Device> g_d3dDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> g_d3dDeviceContext;
Microsoft::WRL::ComPtr<IDXGISwapChain> g_dxgiSwapChain;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> g_basicRTV;
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> g_basicDSV;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	if (!InitWnd(hInstance, nCmdShow)) {
		return 0;
	}

	if (!InitDirectX()) {
		return 0;
	}

	return Run();
}

bool InitWnd(HINSTANCE hInstance, int nCmdShow)
{
	// Objectives:
	// 1. Register window class
	// 2. Create window
	// 3. Show window

	/// 1. Register window class
	WNDCLASS wc;
	wc.style = NULL;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"Ryu Engine";

	RegisterClass(&wc);

	/// 2. Create window
	RECT rect{ 0, 0, CLIENT_WIDTH, CLIENT_HEIGHT };
	AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU, false);

	g_hMainWnd = CreateWindow(
		L"Ryu Engine",
		L"Ryu Engine",
		WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		NULL,
		NULL,
		hInstance,
		nullptr
	);

	/// 3. Show window
	ShowWindow(g_hMainWnd, nCmdShow);

	return true;
}

bool InitDirectX()
{
	// Objectives:
	// 1. Create device and device context
	// 2. Create swapchain
	// 3. Create render-target view
	// 4. Create depth-stencil buffer and view

	/// 1. Create device and device context
	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	array<D3D_FEATURE_LEVEL, 6> featureLevels{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	if (FAILED(D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		deviceFlags,
		featureLevels.data(),
		(UINT)featureLevels.size(),
		D3D11_SDK_VERSION,
		g_d3dDevice.GetAddressOf(),
		nullptr,
		g_d3dDeviceContext.GetAddressOf()
	))) {
		return false;
	}

	/// 2. Create swapchain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = g_hMainWnd;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
	if (FAILED(g_d3dDevice.As(&dxgiDevice))) {
		return false;
	}

	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
	if (FAILED(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()))) {
		return false;
	}

	Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
	if (FAILED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void **)dxgiFactory.GetAddressOf()))) {
		return false;
	}
	
	if (FAILED(dxgiFactory->CreateSwapChain(
		g_d3dDevice.Get(),
		&swapChainDesc,
		g_dxgiSwapChain.GetAddressOf()
	))) {
		return false;
	}

	/// 3. Create render-target view
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	if (FAILED(g_dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)backBuffer.GetAddressOf()))) {
		return false;
	}

	if (FAILED(g_d3dDevice->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		g_basicRTV.GetAddressOf()
	))) {
		return false;
	}

	// Create a depth stencil buffer and view
	D3D11_TEXTURE2D_DESC backBufferDesc;
	backBuffer->GetDesc(&backBufferDesc);

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));
	depthStencilDesc.Width = backBufferDesc.Width;
	depthStencilDesc.Height = backBufferDesc.Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
	if (FAILED(g_d3dDevice->CreateTexture2D(
		&depthStencilDesc,
		nullptr,
		depthStencilBuffer.GetAddressOf()
	))) {
		return false;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	depthStencilViewDesc.Format = depthStencilDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	if (FAILED(g_d3dDevice->CreateDepthStencilView(
		depthStencilBuffer.Get(),
		&depthStencilViewDesc,
		g_basicDSV.GetAddressOf()
	))) {
		return false;
	}

	return true;
}

void Render()
{
	// Objectives:
	// 1. Configure pipeline
	// 2. Draw
	// 3. Present

	/// 1. Configure pipeline
	g_d3dDeviceContext->RSSetViewports(1, &BASIC_VIEWPORT);
	g_d3dDeviceContext->OMSetRenderTargets(1, g_basicRTV.GetAddressOf(), g_basicDSV.Get());

	/// 2. Draw

	/// 3. Present
	g_dxgiSwapChain->Present(0, 0);
}

int Run()
{
	MSG msg{};

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			Render();
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}