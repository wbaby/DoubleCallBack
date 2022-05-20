typedef enum DXGI_FORMAT
{
	DXGI_FORMAT_UNKNOWN = 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
	DXGI_FORMAT_R32G32B32A32_UINT = 3,
	DXGI_FORMAT_R32G32B32A32_SINT = 4,
	DXGI_FORMAT_R32G32B32_TYPELESS = 5,
	DXGI_FORMAT_R32G32B32_FLOAT = 6,
	DXGI_FORMAT_R32G32B32_UINT = 7,
	DXGI_FORMAT_R32G32B32_SINT = 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
	DXGI_FORMAT_R16G16B16A16_UNORM = 11,
	DXGI_FORMAT_R16G16B16A16_UINT = 12,
	DXGI_FORMAT_R16G16B16A16_SNORM = 13,
	DXGI_FORMAT_R16G16B16A16_SINT = 14,
	DXGI_FORMAT_R32G32_TYPELESS = 15,
	DXGI_FORMAT_R32G32_FLOAT = 16,
	DXGI_FORMAT_R32G32_UINT = 17,
	DXGI_FORMAT_R32G32_SINT = 18,
	DXGI_FORMAT_R32G8X24_TYPELESS = 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
	DXGI_FORMAT_R10G10B10A2_UNORM = 24,
	DXGI_FORMAT_R10G10B10A2_UINT = 25,
	DXGI_FORMAT_R11G11B10_FLOAT = 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
	DXGI_FORMAT_R8G8B8A8_UNORM = 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
	DXGI_FORMAT_R8G8B8A8_UINT = 30,
	DXGI_FORMAT_R8G8B8A8_SNORM = 31,
	DXGI_FORMAT_R8G8B8A8_SINT = 32,
	DXGI_FORMAT_R16G16_TYPELESS = 33,
	DXGI_FORMAT_R16G16_FLOAT = 34,
	DXGI_FORMAT_R16G16_UNORM = 35,
	DXGI_FORMAT_R16G16_UINT = 36,
	DXGI_FORMAT_R16G16_SNORM = 37,
	DXGI_FORMAT_R16G16_SINT = 38,
	DXGI_FORMAT_R32_TYPELESS = 39,
	DXGI_FORMAT_D32_FLOAT = 40,
	DXGI_FORMAT_R32_FLOAT = 41,
	DXGI_FORMAT_R32_UINT = 42,
	DXGI_FORMAT_R32_SINT = 43,
	DXGI_FORMAT_R24G8_TYPELESS = 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
	DXGI_FORMAT_R8G8_TYPELESS = 48,
	DXGI_FORMAT_R8G8_UNORM = 49,
	DXGI_FORMAT_R8G8_UINT = 50,
	DXGI_FORMAT_R8G8_SNORM = 51,
	DXGI_FORMAT_R8G8_SINT = 52,
	DXGI_FORMAT_R16_TYPELESS = 53,
	DXGI_FORMAT_R16_FLOAT = 54,
	DXGI_FORMAT_D16_UNORM = 55,
	DXGI_FORMAT_R16_UNORM = 56,
	DXGI_FORMAT_R16_UINT = 57,
	DXGI_FORMAT_R16_SNORM = 58,
	DXGI_FORMAT_R16_SINT = 59,
	DXGI_FORMAT_R8_TYPELESS = 60,
	DXGI_FORMAT_R8_UNORM = 61,
	DXGI_FORMAT_R8_UINT = 62,
	DXGI_FORMAT_R8_SNORM = 63,
	DXGI_FORMAT_R8_SINT = 64,
	DXGI_FORMAT_A8_UNORM = 65,
	DXGI_FORMAT_R1_UNORM = 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
	DXGI_FORMAT_BC1_TYPELESS = 70,
	DXGI_FORMAT_BC1_UNORM = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB = 72,
	DXGI_FORMAT_BC2_TYPELESS = 73,
	DXGI_FORMAT_BC2_UNORM = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB = 75,
	DXGI_FORMAT_BC3_TYPELESS = 76,
	DXGI_FORMAT_BC3_UNORM = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB = 78,
	DXGI_FORMAT_BC4_TYPELESS = 79,
	DXGI_FORMAT_BC4_UNORM = 80,
	DXGI_FORMAT_BC4_SNORM = 81,
	DXGI_FORMAT_BC5_TYPELESS = 82,
	DXGI_FORMAT_BC5_UNORM = 83,
	DXGI_FORMAT_BC5_SNORM = 84,
	DXGI_FORMAT_B5G6R5_UNORM = 85,
	DXGI_FORMAT_B5G5R5A1_UNORM = 86,
	DXGI_FORMAT_B8G8R8A8_UNORM = 87,
	DXGI_FORMAT_B8G8R8X8_UNORM = 88,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
	DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
	DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
	DXGI_FORMAT_BC6H_TYPELESS = 94,
	DXGI_FORMAT_BC6H_UF16 = 95,
	DXGI_FORMAT_BC6H_SF16 = 96,
	DXGI_FORMAT_BC7_TYPELESS = 97,
	DXGI_FORMAT_BC7_UNORM = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB = 99,
	DXGI_FORMAT_AYUV = 100,
	DXGI_FORMAT_Y410 = 101,
	DXGI_FORMAT_Y416 = 102,
	DXGI_FORMAT_NV12 = 103,
	DXGI_FORMAT_P010 = 104,
	DXGI_FORMAT_P016 = 105,
	DXGI_FORMAT_420_OPAQUE = 106,
	DXGI_FORMAT_YUY2 = 107,
	DXGI_FORMAT_Y210 = 108,
	DXGI_FORMAT_Y216 = 109,
	DXGI_FORMAT_NV11 = 110,
	DXGI_FORMAT_AI44 = 111,
	DXGI_FORMAT_IA44 = 112,
	DXGI_FORMAT_P8 = 113,
	DXGI_FORMAT_A8P8 = 114,
	DXGI_FORMAT_B4G4R4A4_UNORM = 115,

	DXGI_FORMAT_P208 = 130,
	DXGI_FORMAT_V208 = 131,
	DXGI_FORMAT_V408 = 132,


	DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
	DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,


	DXGI_FORMAT_FORCE_UINT = 0xffffffff
} DXGI_FORMAT;
typedef struct DXGI_SAMPLE_DESC
{
	UINT Count;
	UINT Quality;
} DXGI_SAMPLE_DESC;
typedef enum D3D11_USAGE
{
	D3D11_USAGE_DEFAULT = 0,
	D3D11_USAGE_IMMUTABLE = 1,
	D3D11_USAGE_DYNAMIC = 2,
	D3D11_USAGE_STAGING = 3
} 	D3D11_USAGE;
typedef struct D3D11_TEXTURE2D_DESC
{
	UINT Width;
	UINT Height;
	UINT MipLevels;
	UINT ArraySize;
	DXGI_FORMAT Format;
	DXGI_SAMPLE_DESC SampleDesc;
	D3D11_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
} 	D3D11_TEXTURE2D_DESC;
typedef enum D3D11_CPU_ACCESS_FLAG
{
	D3D11_CPU_ACCESS_WRITE = 0x10000L,
	D3D11_CPU_ACCESS_READ = 0x20000L
} 	D3D11_CPU_ACCESS_FLAG;
typedef struct D3D11_MAPPED_SUBRESOURCE
{
	PVOID Data;
	UINT RowPitch;
	UINT DepthPitch;
} 	D3D11_MAPPED_SUBRESOURCE;
enum D3D11_MAP
{
	D3D11_MAP_READ = 1,
	D3D11_MAP_WRITE = 2,
	D3D11_MAP_READ_WRITE = 3,
	D3D11_MAP_WRITE_DISCARD = 4,
	D3D11_MAP_WRITE_NO_OVERWRITE = 5
} D3D11_MAP;



class DirectX {
private:
	PVOID DescBuffer;
	PVOID EntityBuffer;
	
	PVOID SwapChain;
	PVOID Device;
	PVOID Context;
	PVOID Surface;

	//HWND Hwnd;

	__forceinline PVOID GetVFunc(PVOID Class, ULONG Index) {
		return (*(PVOID**)Class)[Index];
	}

	__forceinline void Release(PVOID Class) {
		UserCallback2.Call(GetVFunc(Class, 2), Class, 0ull, 0ull, 0ull);
	}


public:
	DirectX()
	{
		SwapChain = nullptr;
	}

	void UpdateDesktopWnd() {
		//auto user32 = GetUserModuleBase(IoGetCurrentProcess(), "user32.dll");
		//auto RedrawWindow1 = GetProcAdress(user32, "InvalidateRect");
		//UserCallback.Call(RedrawWindow1, 0ull, 0ull, 1ull);
	}

	bool Init(PVOID Swap)
	{
		if (Swap == SwapChain) {
			//if (!IsAddrValid((u64)Surface))
			//	return false;
			return true;
		}


		DescBuffer = UAlloc(0x1000);
		EntityBuffer = UAlloc(0x1000);

		SwapChain = Swap;

		auto DescBuffer = UAlloc(0x1000);
		auto EntityBuffer = UAlloc(0x1000);

		//UserCallback2.Call(GetVFunc(SwapChain, 7), SwapChain, DescBuffer, EntityBuffer);


		const GUID ID3D11DeviceVar = { 0xdb6f6ddb, 0xac77, 0x4e88, 0x82, 0x53, 0x81, 0x9d, 0xf9, 0xbb, 0xf1, 0x40 };
		MemCpy(DescBuffer, (PVOID)&ID3D11DeviceVar, sizeof(ID3D11DeviceVar));
		UserCallback2.Call(GetVFunc(SwapChain, 7), SwapChain, DescBuffer, EntityBuffer);
		Device = *(PVOID*)EntityBuffer;
		Release(Device);



		UserCallback2.Call(GetVFunc(Device, 40), Device, EntityBuffer);
		Context = *(PVOID*)EntityBuffer;
		Release(Context);

		const GUID ID3D11Texture2DVar = { 0x6f15aaf2, 0xd208, 0x4e89, 0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c };
		MemCpy(DescBuffer, (PVOID)&ID3D11Texture2DVar, sizeof(ID3D11Texture2DVar));
		UserCallback2.Call(GetVFunc(SwapChain, 9), SwapChain, 0, DescBuffer, EntityBuffer);
		Surface = *(PVOID*)EntityBuffer;
		Release(Surface);

		//UFree(DescBuffer);
		//UFree(EntityBuffer);

		return true;
	}
	//HWND GetSwapChainHandle() {
	//	if (!IsAddressValid(SwapChain) && !Init()) return 0;
	//	else return Hwnd;
	//}
	UINT Cnt()
	{
		UserCallback2.Call(GetVFunc(SwapChain, 15), SwapChain, DescBuffer);
		return *(UINT*)DescBuffer;
	}

	void Tick(/*ULONG* Texture, ULONG& Width, ULONG& Height*/) {
		//if (!IsAddressValid(SwapChain))
		
		//auto DescBuffer = UAlloc(0x1000);
		//auto EntityBuffer = UAlloc(0x1000);


		//return;

		D3D11_TEXTURE2D_DESC SDesc; MemZero(&SDesc, sizeof(D3D11_TEXTURE2D_DESC));
		UserCallback2.Call(GetVFunc(Surface, 10), Surface, DescBuffer);

		//return;
	
		SDesc = *(D3D11_TEXTURE2D_DESC*)DescBuffer;
		
		SDesc.BindFlags = 0;
		SDesc.MiscFlags = 0;
		SDesc.Usage = D3D11_USAGE_STAGING;
		SDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

		MemCpy(DescBuffer, &SDesc, sizeof(SDesc));
		UserCallback2.Call(GetVFunc(Device, 5), Device, DescBuffer, 0, EntityBuffer);

		PVOID TmpTex = *(PVOID*)EntityBuffer;
		UserCallback2.Call(GetVFunc(Context, 47), Context, TmpTex, Surface);

		D3D11_MAPPED_SUBRESOURCE MapRes; MemZero(&MapRes, sizeof(D3D11_MAPPED_SUBRESOURCE));
		UserCallback2.Call(GetVFunc(Context, 14), Context, TmpTex, 0, D3D11_MAP_READ_WRITE, 0, DescBuffer);
		MapRes = *(D3D11_MAPPED_SUBRESOURCE*)DescBuffer;
		

		//__stosd((PDWORD)MapRes.Data, __rdtsc(), 200);
		
		//for (int Y = 0; Y < SDesc.Height; Y++)
		//{
		//	int RowStart = Y * MapRes.RowPitch / 4;
		//	for (int X = 0; X < SDesc.Width; X++)
		//	{
		//		//auto Pixel = Texture[Y * SDesc.Width + X];
		//		auto ggg = ((PULONG)MapRes.Data)[RowStart + X];
		//		//FColor ggg11(ggg);
		//		//ggg11.RGBA[3] = 0xFF;
		//		((PULONG)MapRes.Data)[RowStart + X] = __rdtsc();
		//	}
		//}

		ByteRender rend;
		rend.Setup(SDesc.Width, SDesc.Height, MapRes.Data);
		rend.FillCircle({ 100, 100 }, FColor(__rdtsc()), 100);
		rend.String({ 100, 100 }, L"Hi Im FACE", 1, FColor(__rdtsc()));

		UserCallback2.Call(GetVFunc(Context, 15), Context, TmpTex);
		UserCallback2.Call(GetVFunc(Context, 47), Context, Surface, TmpTex);

	//ret:
		Release(TmpTex);

		//UserCallback.Call(GetVFunc(Context, 111), Context);

		//UFree(DescBuffer);
		//UFree(EntityBuffer);
	}
} DirectX;