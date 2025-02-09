#pragma once

#include <Windows.h>
#include <string>
#include <functional>

namespace Platform
{

	class Window
	{
	public:
		Window();
		~Window();

		bool Initialize(int width, int height, const std::string& title);
		void Terminate();

		void TriggerDisplayChangeCallback();

		HWND GetHandle() const;
		void SetDisplayChangeCallback(std::function<void()> callback);


	private:
		HWND        m_hWnd;
		HINSTANCE   m_hInstance;

		std::function<void()> m_displayChangeCallback;

		// Win32 �̃E�B���h�E�v���V�[�W��
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};

}
