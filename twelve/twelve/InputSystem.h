#pragma once
#include <DirectXMath.h>
#include <Windows.h>

#include "XMFLOAT_Helper.h"

enum ButtonState
{
	None,
	Pressed,
	Released,
	Hold
};

class KeyboardState
{
public:
	friend class InputSystem;

	ButtonState GetKeyState(BYTE keycode) const;
	bool GetKeyDown(BYTE keycode) const;

private:
	BYTE m_prevState[256];
	BYTE m_currentState[256];
};

class MouseState
{
public:
	friend class InputSystem;

	const DirectX::XMFLOAT2& GetPosition() const { return m_position; }
	const DirectX::XMFLOAT2& GetDelta() const;
	const DirectX::XMFLOAT2& GetCenter() const { return m_center; }

private:
	DirectX::XMFLOAT2 m_position;
	DirectX::XMFLOAT2 m_center;
};

struct InputState
{
	KeyboardState keyboard;
	MouseState mouse;
};

class InputSystem
{
public:
	InputSystem();

	bool Initialize(HWND hWind);
	void Update(enum State state);

	const InputState& GetState() const { return m_state; }

private:
	InputState m_state;
};
