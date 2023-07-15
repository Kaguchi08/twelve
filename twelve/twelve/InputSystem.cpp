#include "InputSystem.h"

ButtonState KeyboardState::GetKeyState(BYTE keycode) const
{
	if (current_state_[keycode] & 0x80)
	{
		if (prev_state_[keycode] & 0x80)
		{
			return ButtonState::kHeld;
		}
		else
		{
			return ButtonState::kPressed;
		}
	}
	else
	{
		if (prev_state_[keycode] & 0x80)
		{
			return ButtonState::kReleased;
		}
		else
		{
			return ButtonState::kNone;
		}
	}
}

bool KeyboardState::GetKeyDown(BYTE keycode) const
{
	return current_state_[keycode] & 0x80;
}

bool InputSystem::Initialize()
{
	// リセット
	memset(state_.keyboard.prev_state_, 0x00, 256);
	memset(state_.keyboard.current_state_, 0x00, 256);

	return true;
}

void InputSystem::Update()
{
	// キーボードの状態を更新
	memcpy(state_.keyboard.prev_state_, state_.keyboard.current_state_, 256);
	GetKeyboardState(state_.keyboard.current_state_);
}
