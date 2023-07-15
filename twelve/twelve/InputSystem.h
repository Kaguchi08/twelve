#pragma once
#include "XMFLOAT_Helper.h"
#include <Windows.h>
#include <DirectXMath.h>
#include <unordered_map>

enum ButtonState {
	kNone,
	kPressed,
	kReleased,
	kHeld
};

class KeyboardState
{
public:
	friend class InputSystem;

	ButtonState GetKeyState(BYTE keycode) const;
	bool GetKeyDown(BYTE keycode) const;

private:
	BYTE prev_state_[256];
	BYTE current_state_[256];
};

class MouseState
{

};

struct InputState
{
	KeyboardState keyboard;
	MouseState mouse;
};

class InputSystem
{
public:
	bool Initialize();
	void Update();

	const InputState& GetState() const { return state_; }
private:
	InputState state_;
};

