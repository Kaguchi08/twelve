#pragma once

#include <Windows.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace Platform
{

	using Microsoft::WRL::ComPtr;

	struct HDRInfo
	{
		bool supportsHDR;
		float maxDisplayLuminance;
		float minDisplayLuminance;
	};

	class DisplayManager
	{
	public:
		HDRInfo QueryHDRSupport(HWND hWnd);
	};

} // namespace Platform
