//--------------------------------------------------------------------------------------
// File: Tutorial01.cpp
//
// This application demonstrates creating a Direct3D 11 device
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729718.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11_1.h>
#include <directxcolors.h>
#include "resource.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;      // Global Handle Instance.
HWND                    g_hWnd = nullptr;       // Global Handle Window.
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = nullptr;             // Device: 렌더링에 필요한 리소스들(버퍼, 텍스쳐, 버텍스세이더 등)을 생성하기 위한 인터페이스 입니다.
ID3D11Device1*          g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*    g_pImmediateContext = nullptr;      // Context: Device가 소유한 리소스를 사용하여, 어떤 파이프라인 상태에서 / 어떤 렌더링 커맨드를 내리기 위한 인터페이스입니다.
ID3D11DeviceContext1*   g_pImmediateContext1 = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
IDXGISwapChain1*        g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    // Window 창 생성.
    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    // DX Device 구성. Body를 생성한다.
    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 1: Direct3D 11 Basics",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
// 1. Device 생성: 사용할 레이어등에 관한 Flag와 드라이버에 맞는 Device 생성
// 2. SwapChain 생성: DXGI_SWAP_CHAIN_DESC 에 명시하여 생성.
// 3. ImmediateContext 설정: Device 생성 시 획득한 ImmediateContext에 렌더타겟과 뷰포트 설정.
HRESULT InitDevice()
{
    HRESULT hr = S_OK;
    
    // 먼저 생성해둔 윈도우의 Rect.
    RECT rc;
    GetClientRect( g_hWnd, &rc );          
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // 1. Device 생성: 사용할 레이어등에 관한 Flag와 드라이버에 맞는 Device 생성
    
    // D3D Device에서 사용될 레이어의 플래그를 설정
    UINT createDeviceFlags = 0;
    /*
    typedef enum D3D11_CREATE_DEVICE_FLAG {
        D3D11_CREATE_DEVICE_SINGLETHREADED = 0x1,                                   // App이 싱글스레드로 동작하는 경우, 성능을 향상시킬 수 있습니다.
        D3D11_CREATE_DEVICE_DEBUG = 0x2,                                            // Debug Layer가 동작합니다. 유효성검사, 변수 일관성 검사, 에러 리포트 등으로 속도가 느려집니다.
        D3D11_CREATE_DEVICE_SWITCH_TO_REF = 0x4,                                    // Direct3D 11에서 지원되지 않습니다.
        D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS = 0x8,         // 여러 스레드가 만들어지는 것을 방지합니다. 사용하지 않는 것이 좋다는데, WARP의 작동과 관련이 있어 보입니다.
        D3D11_CREATE_DEVICE_BGRA_SUPPORT = 0x20,                                    // Direct3D 리소스와의 Direct2D 상호 운용성에 필요합니다.
        D3D11_CREATE_DEVICE_DEBUGGABLE = 0x40,                                      // 셰이더 디버깅에 사용할 수 있는 정보를 유지하도록 합니다.
        D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY = 0x80,   // 릴리즈 빌드를 사용하는 최종 사용자가, DirectX 제어판 등을 사용해 Debug Layer을 사용하여 모니터링 하지 못하게 합니다.
        D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT = 0x100,                            // GPU Timeout이 걸리지 않도록 합니다. (2초) 
        D3D11_CREATE_DEVICE_VIDEO_SUPPORT = 0x800
       }
    // https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ne-d3d11-d3d11_create_device_flag
    */


#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    /*
    typedef
    enum D3D_DRIVER_TYPE
    {
        D3D_DRIVER_TYPE_UNKNOWN	= 0,
        D3D_DRIVER_TYPE_HARDWARE	= ( D3D_DRIVER_TYPE_UNKNOWN + 1 ) ,     // GPU 하드웨어 가속을 사용합니다.
        D3D_DRIVER_TYPE_REFERENCE	= ( D3D_DRIVER_TYPE_HARDWARE + 1 ) ,    // 개발 중 디버그 지원을 위한 소프트웨어로 구현된 드라이버 입니다. GPU가 없어도 동작할 수 있습니다.
        D3D_DRIVER_TYPE_NULL	= ( D3D_DRIVER_TYPE_REFERENCE + 1 ) ,       // 렌더링 기능을 제외한 소프트웨어
        D3D_DRIVER_TYPE_SOFTWARE	= ( D3D_DRIVER_TYPE_NULL + 1 ) ,        // 예약만 되어있고 사용하지 않습니다.
        D3D_DRIVER_TYPE_WARP	= ( D3D_DRIVER_TYPE_SOFTWARE + 1 )          // 고성능 소프트웨어 래스터라이저인 WARP를 사용합니다. CPU가 현대적일 경우, REFERENCE 보다 훨씬 나은 성능을 보여줍니다.
    } 	D3D_DRIVER_TYPE;
    */

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    /*
    * GPU에서 지원하는 D3D Level 입니다.
    */
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    // driverTypes[] 에 넣어둔 D3D_DRIVER_TYPE에 따라 Device를 생성합니다. 생성 실패시 다음 단계의 D3D_DRIVER_TYPE으로 생성을 시도합니다.
    // 해당 코드는 Hardware->WARP->Ref 순으로 시도합니다.
    /*
    HRESULT D3D11CreateDevice(
        __in   IDXGIAdapter             *pAdapter,              // 디스플레이 디바이스(비디오카드)를 지정합니다. null일 경우 최초로 발견한 디바이스를 사용합니다.
        __in   D3D_DRIVER_TYPE          DriverType,             // 생성할 타입을 넣습니다. 해당 코드는 Hardware->WARP->Ref 순으로 시도합니다.
        __in   HMODULE                  Software,               // Software Rasterizer 가 구현된 모듈의 핸들을 넣습니다. 일반적으로는 null을 넣습니다.
        __in   UINT                     Flags,                  // 위에서 설정한 디바이스의 레이어 플래그를 넣습니다.
        __in   const D3D_FEATURE_LEVEL  *pFeatureLevels,        // 지원가능한 레벨에 대한 Array입니다.
        __in   UINT                     FeatureLevels,
        __in   UINT                     SDKVersion,             // DX SDK의 버전입니다.
        __out  ID3D11Device             **ppDevice,             // 생성 성공된 디바이스의 포인터입니다.
        __out  D3D_FEATURE_LEVEL        *pFeatureLevel,         // 생성 성공된 지원가능 레벨입니다.
        __out  ID3D11DeviceContext      **ppImmediateContext    // 생성 성공된 디바이스 컨텍스트입니다.
        );

        //https://vsts2010.tistory.com/512
    */

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );

        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                                    D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        }

        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // ~ 1. Device 생성: 사용할 레이어등에 관한 Flag와 드라이버에 맞는 Device 생성

    // 2. SwapChain 생성: DXGI_SWAP_CHAIN_DESC 에 명시하여 생성.

    // SwapChain을 생성하는 IDXGIFactory 를 생성합니다.
    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface( __uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice) );
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent( __uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory) );
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    // Create swap chain

    // 11.1 이후 버전일 경우.
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface( __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2) );
    if ( dxgiFactory2 )
    {
        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface( __uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1) );
        if (SUCCEEDED(hr))
        {
            (void) g_pImmediateContext->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1) );
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd( g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1 );
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain) );
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DXGI_SWAP_CHAIN_DESC: SwapChain(버퍼)를 만들기 위한 설정정보 구조체.

        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;   // FrameRate 분자. 
        sd.BufferDesc.RefreshRate.Denominator = 1;  // FrameRate 분모. 59.94 Hz 일 경우는 60000/1001
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   // 해당 버퍼의 사용처.
        sd.OutputWindow = g_hWnd;       // 어느 윈도우에 투사할 것인지.
        sd.SampleDesc.Count = 1;        // 멀티샘플링에 사용할 표본갯수 1,4,8..32(MAX) 멀티샘플링: https://codingfarm.tistory.com/420, https://lipcoder.tistory.com/22
        sd.SampleDesc.Quality = 0;      // 멀티샘플링 품질 레벨 - 0일 경우 하지 않음.
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain( g_pd3dDevice, &sd, &g_pSwapChain );
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation( g_hWnd, DXGI_MWA_NO_ALT_ENTER );

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    // ~ 2. SwapChain 생성: DXGI_SWAP_CHAIN_DESC 에 명시하여 생성.

    // 3. ImmediateContext 설정: Device 생성 시 획득한 ImmediateContext에 렌더타겟과 뷰포트 설정.

    // SwapChain의 백버퍼를 가져옵니다.
    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;

    // SwapChain에서 얻은 백버퍼를 사용한 렌더타겟뷰를 만듭니다. (현재는 백버퍼에 아무것도 없지만...)
    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();     // Q.계속 쓰게될 백버퍼를 왜 Release하는지...refCount만 하나 내리는건지 애매합니다.
    if( FAILED( hr ) )
        return hr;

    // ImmediateContext 설정: 위에서 만든 렌더타겟뷰를 타겟으로.
    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, nullptr );

    // Setup the viewport
    // 뷰포트 설정. 렌더타겟뷰의 그려질 화면영역.
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    // Immediate 컨텍스트 설정: 뷰포트 설정.
    g_pImmediateContext->RSSetViewports( 1, &vp );      // RS 는 Rasterize Stage.

    // ~3. ImmediateContext 설정: Device 생성 시 획득한 ImmediateContext에 렌더타겟과 뷰포트 설정.

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
    // Just clear the backbuffer
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, Colors::MidnightBlue );
    g_pSwapChain->Present( 0, 0 );
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain1 ) g_pSwapChain1->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice1 ) g_pd3dDevice1->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}
