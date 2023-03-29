# D3D11Study - Tutorial_1

## 개요
----------------
D3D11을 사용하기 위하여, 드로우할 Window창 생성 및 Device, SwapChain, Context를 생성하고, Render루프를 구성합니다.
D3D11는 렌더링에 필요한 리소스 생성과 렌더링커맨드를 분리하였습니다. 이는 각각, Device, DeviceContext 가 이를 대표합니다.

ID3D11Device :
>	>	렌더링에 필요한 리소스들(버퍼, 텍스쳐, 버텍스셰이더..)등을 생성하기 위해 D3D에서 제공하는 인터페이스입니다.
	
ID3D11DeviceContext :
>	Device에서 생성되는 요소들을 활용하여, 각 렌더링파이프라인 Stage에 맞는 렌더링커맨드를 생성하는 매서드를 제공하는 인터페이스입니다.
>	ImmediateContext는 하나의 App당 단 하나를 가지고 있습니다. 이는 즉각적인 렌더링커맨드를 보냅니다.
>	DeferredContext는 멀티스레딩을 위한 컨텍스트입니다. 커맨드리스트를 기록하고 있습니다.

(참고: MS Direct3D 11의 디바이스 소개
<https://learn.microsoft.com/ko-kr/windows/win32/direct3d11/overviews-direct3d-11-devices-intro>)

깜빡임 및 테어링 현상을 해결하기 위해 백버퍼가 필요합니다.
D3D에서는 SwapChain 안에 백버퍼를 두고 있습니다.

IDXGISwapChain :
>	Direct3D에서 SwapChain을 대표하는 인터페이스입니다. 프론트/백 버퍼를 가지고 있고, 버퍼에 대한 매서드를 제공합니다.

(참고: MS 스왑 체인이란? (Direct3D 9)
<https://learn.microsoft.com/ko-kr/windows/win32/direct3d9/what-is-a-swap-chain->)

## 튜토리얼 프로그램의 메인함수
----------------
윈도우 생성, D3D 사용 개체 준비, 게임루프(렌더) 로 이루어진 기본적인 메인함수입니다.

```cpp
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    // Window 창 생성.
    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    // D3D Device, SwapChain, Context 구성.
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
            // 렌더링.
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}
```

## InitDevice()
-------------------

D3D11을 사용하기 위해 Device, SwapChain, Context 를 구성하는 InitDevice() 를 부분적으로 나누어 살펴 볼 것입니다.

### 1. Device 생성
사용할 레이어등에 관한 Flag와 드라이버에 맞는 Device 생성을 위해 파라미터들을 준비합니다.
#### DeviceFlag
```cpp
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
    */
    
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
```
(참고: MS D3D11_CREATE_DEVICE_FLAG 열거형(d3d11.h)
<https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ne-d3d11-d3d11_create_device_flag>)

#### DriverTypes
```cpp
D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

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

    UINT numDriverTypes = ARRAYSIZE( driverTypes );
```
(참고: MS D3D_DRIVER_TYPE 열거형(d3dcommon.h) 
<https://learn.microsoft.com/ko-kr/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_driver_type>)

#### Feature Level
```cpp
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
```

#### D3D11CreateDevice
위 파라미터로 사용할 변수들을 이용하여, Device를 생성합니다.
튜토리얼은 D3D_DRIVER_TYPE 이 Hardware->WARP->Ref 순서로 시도하여, 성공할 때 까지 진행합니다.
```cpp
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
```
(참고: D3D11CreateDevice의 각 파라미터에 대한 설명
<https://vsts2010.tistory.com/512>)

### 2. SwapChain 생성
IDXGIFactory1 에서 생성메서드를 제공하며, 이는 Device->GetAdapter()->GetParent()를 통해 구할 수 있습니다.
```cpp
    // SwapChain을 생성하는 IDXGIFactory 를 구합니다.
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
```

DXGI_SWAP_CHAIN_DESC 에 SwapChain의 스펙을 명세하여 이를 파라미터로 전달하여 생성합니다.
튜토리얼에서는 11.1 이후 버전과 그 이전으로 나누어 코드가 작성되어 있습니다.
버전별로 Factory가 다르지만 큰 차이가 없으므로, 11.0 에서 SwapChain을 생성하는 내용을 살펴봅니다.

```cpp
        // DirectX 11.0 systems
        // DXGI_SWAP_CHAIN_DESC: SwapChain(버퍼)를 만들기 위한 설정정보 구조체.
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;   // FrameRate 분자. 
        sd.BufferDesc.RefreshRate.Denominator = 1;  // FrameRate 분모. 59.94 Hz 일 경우는 60000/1001
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   // 해당 버퍼의 사용처.
        sd.OutputWindow = g_hWnd;       // 어느 윈도우에 투사할 것인지.
        sd.SampleDesc.Count = 1;        // 멀티샘플링에 사용할 표본갯수 1,4,8..32(MAX)
        sd.SampleDesc.Quality = 0;      // 멀티샘플링 품질 레벨 - 0일 경우 하지 않음.
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain( g_pd3dDevice, &sd, &g_pSwapChain );
```
(참고: 멀티샘플링
<https://codingfarm.tistory.com/420>, 
<https://lipcoder.tistory.com/22>)

### 3. ImmediateContext 설정
Device 생성 시 획득한 ImmediateContext에 렌더타겟뷰와 뷰포트를 설정합니다.

백버퍼를 SwapChain에서 가져온 후, 이를 타겟으로 하는 렌더타겟뷰를 만듭니다. 그리고 이를 Context에 설정합니다.
```cpp
// SwapChain의 백버퍼를 가져옵니다.
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;
        
    // SwapChain에서 얻은 백버퍼를 사용한 렌더타겟뷰를 만듭니다. (현재는 백버퍼에 아무것도 없지만...)
    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();     // Q.계속 쓰게 될 백버퍼를 왜 Release하는지...refCount만 하나 내리는건지 애매합니다.
    if( FAILED( hr ) )
        return hr;

    // ImmediateContext 설정: 위에서 만든 렌더타겟뷰를 타겟으로.
    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, nullptr );
```

화면에 그려질 뷰포트를 설정하고, 이를 Context에 설정합니다.
```cpp
    // Setup the viewport
    // 뷰포트 설정. 렌더타겟뷰가 그려질 화면영역.
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    // Immediate 컨텍스트 설정: 뷰포트 설정.
    g_pImmediateContext->RSSetViewports( 1, &vp );      // RS 는 Rasterize Stage.
```

이로써 InitDevice() 의 내용이 끝나고, 기본적인 준비가 되었습니다.

## Render()
----------
아직은 그려지는 것이 아무것도 없지만,
루프에서 호출되고 있는 Render() 를 조금 살펴봅니다.
```cpp
    // Just clear the backbuffer
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, Colors::MidnightBlue );
    g_pSwapChain->Present( 0, 0 );
```
현재는 열심히 청소하고 비어있는 버퍼만 투사하고 있습니다.

