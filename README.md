# D3D11Study - Tutorial_6

## 01 개요

Tutorial5_Transform 에 이어서 다음과 같이 진행됩니다.

![Tutorial5_Result](https://github.com/Zeniz/D3D11Study/assets/46617300/70d4b944-b844-4f93-9c1e-08485564832d)
<Tutorial5_Transform의 결과화면>

----------------
![Tutorial6_Result](https://github.com/Zeniz/D3D11Study/assets/46617300/61fed4eb-4b3b-4c04-a029-e763df6442b1)
<Tutorial6_Lighting의 결과화면>

----------------

Lighting 적용 내용은 곧 픽셀 색에 대한 추가연산 적용입니다.

따라서, 픽셀 셰이더에서 어떻게 색을 결정하는지에 대한 내용입니다.

해당 튜토리얼의 진행 개요는 다음과 같습니다.
1. 조명법에 대한 설명 (Lambertian 조명)
2. 고정된 광원, 회전하는 광원에 대한 구현
3. 픽셀셰이더 라이팅 적용 

----------------

## 02 Lambertian 조명

라이팅의 적용은 광원으로부터 반사된 빛의 세기를 계산하는 것과 같습니다.

램버시안 조명은 단순히 광원과 표면과의 사잇각을 빛의 세기로 간주합니다.

![Lambertian_cosine_law](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FotsaW%2FbtqLf7N74wq%2Fea6QuXpFPeadDkRssvo66k%2Fimg.png)

이해를 돕기 위해, 해당 프로젝트의 결과에서 반사체의 회전을 멈춰서 보면 다음과 같습니다.

![Tutorial6_LightRotateOnly](https://github.com/Zeniz/D3D11Study/assets/46617300/fe65bfc9-65a7-4bc8-ae9d-2e0fd810e87e)

----------------

## 03 광원

광원의 방향벡터와 색을 추가합니다.
광원은 고정된 광원과, Y축으로 회전하는 광원 2가지로 구성됩니다.

```cpp
struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
    // 추가: 광원의 방향과 색.
	XMFLOAT4 vLightDir[2];
	XMFLOAT4 vLightColor[2];
	XMFLOAT4 vOutputColor;
};
```

광원의 값을 설정한 후,
회전하는 광원(index == 1)에 대해 회전을 적용합니다.

(vLightDir은 광원으로부터 오브젝트로의 벡터가 아님에 유의바랍니다.
반사체가 원점에 있고, 반사체->광원으로의 벡터로 사용되고 있습니다.)

```cpp
void Render()
{
	// ...
	
	// 광원의 데이터
    // Setup our lighting parameters
    XMFLOAT4 vLightDirs[2] =
    {
        // 고정된 광원의 위치
        XMFLOAT4( -0.577f, 0.577f, -0.577f, 1.0f ),
        // 움직이는 광원의 위치
        XMFLOAT4( 0.0f, 0.0f, -1.0f, 1.0f ),
    };
    XMFLOAT4 vLightColors[2] =
    {
        // 고정된 광원의 색
        XMFLOAT4( 0.5f, 0.5f, 0.5f, 1.0f ),
        // 움직이는 광원의 색
        XMFLOAT4( 0.5f, 0.0f, 0.0f, 1.0f )
    };

    // 움직이는 광원의 회전 적용
    // Rotate the second light around the origin
	
    XMVECTOR vLightDir = XMLoadFloat4(&vLightDirs[1]);      // 광원의 위치를 벡터화한 후,
	XMMATRIX mRotate = XMMatrixRotationY( -2.0f * t );      // Y축회전에 대한 회전행렬을 구해서,
	vLightDir = XMVector3Transform( vLightDir, mRotate );   // 벡터를 회전시킵니다.
	XMStoreFloat4( &vLightDirs[1], vLightDir );             // 계산된 결과를 vLightDirs[1]에 저장합니다.
	
	// ...
}
```

설정된 두 광원을 버퍼에 적용합니다.

```cpp
void Render()
{
	// ...
	
	// 버퍼에 적용합니다.
    // Update matrix variables and lighting variables
    //
    ConstantBuffer cb1;
	cb1.mWorld = XMMatrixTranspose( g_World );
	cb1.mView = XMMatrixTranspose( g_View );
	cb1.mProjection = XMMatrixTranspose( g_Projection );
	
    // 광원 데이터.
	cb1.vLightDir[0] = vLightDirs[0];
	cb1.vLightDir[1] = vLightDirs[1];
	cb1.vLightColor[0] = vLightColors[0];
	cb1.vLightColor[1] = vLightColors[1];
	cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);
	
	g_pImmediateContext->UpdateSubresource( g_pConstantBuffer, 0, nullptr, &cb1, 0, 0 );
	
	// ...
	
``` 

반사체 오브젝트를 그린 후,
각 광원 오브젝트도 그려줍니다.

```cpp
void Render()
{
	// Render the center cube {...}
	
    // 광원 오브젝트 그리기
    // Render each light
    //
    for( int m = 0; m < 2; m++ )
    {
        float Distance = 3.0f;
        float LightObjectScale = 0.2f;
		
        XMMATRIX mLight = XMMatrixTranslationFromVector(Distance * XMLoadFloat4(&vLightDirs[m]));       // 거리를 적용한 위치행렬 생성
		XMMATRIX mLightScale = XMMatrixScaling(LightObjectScale, LightObjectScale, LightObjectScale);   // 스케일링 행렬 생성.
        mLight = mLightScale * mLight;

        // Update the world variable to reflect the current light
		cb1.mWorld = XMMatrixTranspose( mLight );
		cb1.vOutputColor = vLightColors[m];
		g_pImmediateContext->UpdateSubresource( g_pConstantBuffer, 0, nullptr, &cb1, 0, 0 );

		g_pImmediateContext->PSSetShader( g_pPixelShaderSolid, nullptr, 0 );
		g_pImmediateContext->DrawIndexed( 36, 0, 0 );
    }
	
	// ...
}

``` 

----------------

## 04 라이팅 연산 - 픽셀셰이더

02 Lambertian 조명을 적용합니다.

LightIntensity(0 ~ 1) = saturate( dot(표면의 법선벡터, 반사된 빛벡터) )

FinalColor = LightIntensity * vLightColor

```cpp

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;
    
    //do NdotL lighting for 2 lights
    for(int i=0; i<2; i++)
    {
        // 내적으로 구한 사잇각이 빛의 세기를 나타냅니다.
        finalColor += saturate( dot( (float3)vLightDir[i], input.Norm) * vLightColor[i] );
    }
    finalColor.a = 1;
    return finalColor;
}

```
