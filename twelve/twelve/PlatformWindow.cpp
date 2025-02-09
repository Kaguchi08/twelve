#include "PlatformWindow.h"

#include "DisplayManager.h"

namespace Platform
{
	const auto ClassName = TEXT("DirectX");

	Window::Window()
		: m_hWnd(nullptr), m_hInstance(nullptr)
	{
	}

	Window::~Window()
	{
		Terminate();
	}

	bool Window::Initialize(int width, int height, const std::string& title)
	{
		// �C���X�^���X�n���h�����擾
		auto hInst = GetModuleHandle(nullptr);
		if (hInst == nullptr)
		{
			return false;
		}

		// �E�B���h�E�N���X�̓o�^
		WNDCLASSEX wc = {};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = Window::WndProc;
		wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
		wc.hCursor = LoadCursor(hInst, IDC_ARROW);
		wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = ClassName;
		wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

		if (!RegisterClassEx(&wc))
		{
			return false;
		}

		// �C���X�^���X�n���h���̐ݒ�
		m_hInstance = hInst;

		// �E�B���h�E�T�C�Y��ݒ�
		RECT rc = {};
		rc.right = static_cast<LONG>(width);
		rc.bottom = static_cast<LONG>(height);

		// �E�B���h�E�T�C�Y�𒲐�
		auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
		AdjustWindowRect(&rc, style, FALSE);

		// �E�B���h�E�̐���
		m_hWnd = CreateWindowEx(
			0,
			ClassName,
			title.c_str(),
			style,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rc.right - rc.left,
			rc.bottom - rc.top,
			nullptr,
			nullptr,
			m_hInstance,
			nullptr
		);

		if (m_hWnd == nullptr)
		{
			return false;
		}

		// GWLP_USERDATA �� this �|�C���^���Z�b�g���Ă���
		SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		// �E�B���h�E��\��
		ShowWindow(m_hWnd, SW_SHOWNORMAL);

		// �E�B���h�E���X�V
		UpdateWindow(m_hWnd);

		// �E�B���h�E�Ƀt�H�[�J�X��ݒ�
		SetFocus(m_hWnd);

		return true;
	}

	void Window::Terminate()
	{
		if (m_hWnd != nullptr)
		{
			DestroyWindow(m_hWnd);
			m_hWnd = nullptr;
		}

		if (m_hInstance != nullptr)
		{
			UnregisterClass(ClassName, m_hInstance);
			m_hInstance = nullptr;
		}
	}

	void Window::TriggerDisplayChangeCallback()
	{
		if (m_displayChangeCallback)
		{
			m_displayChangeCallback();
		}
	}

	HWND Window::GetHandle() const
	{
		return m_hWnd;
	}

	void Window::SetDisplayChangeCallback(std::function<void()> callback)
	{
		m_displayChangeCallback = callback;
	}


	// �V���v���ȃE�B���h�E�v���V�[�W��
	LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		// GWLP_USERDATA ���� Window �C���X�^���X�̃|�C���^���擾
		Window* pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		switch (message)
		{
			case WM_CREATE:
			{
				// CREATESTRUCT ���� this �|�C���^���擾���A���[�U�f�[�^�ɃZ�b�g
				LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
				pWindow = reinterpret_cast<Window*>(pcs->lpCreateParams);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
			}
			break;

			// �E�B���h�E���ړ�������f�B�X�v���C�ݒ肪�ς�����ꍇ�� HDR �`�F�b�N�p�̃R�[���o�b�N���Ă�
			case WM_MOVE:
			case WM_DISPLAYCHANGE:
				if (pWindow && pWindow->m_displayChangeCallback)
				{
					pWindow->m_displayChangeCallback();
				}

				break;

			case WM_DESTROY:
				PostQuitMessage(0);

				break;

			default:
				break;
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}

} // namespace Platform
