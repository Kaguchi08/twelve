#define NOMINMAX
#include "DisplayManager.h"
#include <algorithm>

namespace Platform
{

	// ユーティリティ関数：矩形の交差面積を計算する
	static int ComputeIntersectionArea
	(
		int ax1, int ay1,
		int ax2, int ay2,
		int bx1, int by1,
		int bx2, int by2
	)
	{
		return std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1))
			* std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
	}

	HDRInfo DisplayManager::QueryHDRSupport(HWND hWnd)
	{
		HDRInfo info = {};
		info.supportsHDR = false;
		info.maxDisplayLuminance = 0.0f;
		info.minDisplayLuminance = 0.0f;

		// DXGI ファクトリーの生成
		ComPtr<IDXGIFactory6> pFactory;
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory))))
		{
			return info;
		}

		// ウィンドウの矩形を取得
		RECT rect = {};
		GetWindowRect(hWnd, &rect);

		ComPtr<IDXGIAdapter1> pAdapter;
		if (FAILED(pFactory->EnumAdapters1(0, &pAdapter)))
		{
			return info;
		}

		// 複数の出力（ディスプレイ）から、ウィンドウと最も重なっているものを探す
		int bestIntersectArea = -1;
		ComPtr<IDXGIOutput> pBestOutput;
		for (UINT i = 0; ; i++)
		{
			ComPtr<IDXGIOutput> pOutput;

			if (pAdapter->EnumOutputs(i, &pOutput) == DXGI_ERROR_NOT_FOUND) break;

			DXGI_OUTPUT_DESC desc = {};
			if (FAILED(pOutput->GetDesc(&desc))) continue;

			int intersectArea = ComputeIntersectionArea(
				rect.left, rect.top, rect.right, rect.bottom,
				desc.DesktopCoordinates.left, desc.DesktopCoordinates.top,
				desc.DesktopCoordinates.right, desc.DesktopCoordinates.bottom
			);
			if (intersectArea > bestIntersectArea)
			{
				bestIntersectArea = intersectArea;
				pBestOutput = pOutput;
			}
		}

		if (!pBestOutput)
		{
			return info;
		}

		// IDXGIOutput6 を問い合わせ、HDR 関連の情報を取得
		ComPtr<IDXGIOutput6> pOutput6;
		if (FAILED(pBestOutput.As(&pOutput6)))
		{
			return info;
		}

		DXGI_OUTPUT_DESC1 desc1 = {};
		if (FAILED(pOutput6->GetDesc1(&desc1)))
		{
			return info;
		}

		info.supportsHDR = (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
		info.maxDisplayLuminance = desc1.MaxLuminance;
		info.minDisplayLuminance = desc1.MinLuminance;

		return info;
	}

} // namespace Platform
