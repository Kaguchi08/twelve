#include "InputSystem.h"

#include "Game.h"

ButtonState KeyboardState::GetKeyState(BYTE keycode) const
{
	if (m_currentState[keycode] & 0x80)
	{
		if (m_prevState[keycode] & 0x80)
		{
			return ButtonState::Hold;
		}
		else
		{
			return ButtonState::Pressed;
		}
	}
	else
	{
		if (m_prevState[keycode] & 0x80)
		{
			return ButtonState::Released;
		}
		else
		{
			return ButtonState::None;
		}
	}
}

bool KeyboardState::GetKeyDown(BYTE keycode) const
{
	return m_currentState[keycode] & 0x80;
}

const DirectX::XMFLOAT2& MouseState::GetDelta() const
{
	return m_position - m_center;
}

InputSystem::InputSystem()
	: m_state()
{
}

bool InputSystem::Initialize(HWND hWind)
{
	// 中央の座標を取得
	RECT rect;
	GetWindowRect(hWind, &rect);
	m_state.mouse.m_center.x = (rect.right - rect.left) / 2;
	m_state.mouse.m_center.y = (rect.bottom - rect.top) / 2;

	SetCursorPos(m_state.mouse.m_center.x, m_state.mouse.m_center.y);
	// カーソルは表示しない
	//ShowCursor(false);

	// リセット
	memset(m_state.keyboard.m_prevState, 0x00, 256);
	memset(m_state.keyboard.m_currentState, 0x00, 256);

	return true;
}

void InputSystem::Update(enum State state)
{
	// キーボードの状態を更新
	memcpy(m_state.keyboard.m_prevState, m_state.keyboard.m_currentState, 256);
	GetKeyboardState(m_state.keyboard.m_currentState);

	// マウスの状態を更新
	POINT current_pos;
	GetCursorPos(&current_pos);
	m_state.mouse.m_position = DirectX::XMFLOAT2(current_pos.x, current_pos.y);

	if (state == State::Play)
	{
		// 中央に戻す
		//SetCursorPos(m_state.mouse.m_center.x, m_state.mouse.m_center.y);
	}
}
