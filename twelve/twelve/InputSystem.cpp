#include "InputSystem.h"
#include "Game.h"

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

const DirectX::XMFLOAT2& MouseState::GetDelta() const
{
	return position_ - center_;
}

InputSystem::InputSystem(Game* game) :
	game_(game),
	state_()
{
}


bool InputSystem::Initialize()
{
	// �����̍��W���擾
	RECT rect;
	GetWindowRect(game_->GetWindowHandle(), &rect);
	state_.mouse.center_.x = (rect.right - rect.left) / 2;
	state_.mouse.center_.y = (rect.bottom - rect.top) / 2;

	SetCursorPos(state_.mouse.center_.x, state_.mouse.center_.y);
	// �J�[�\���͕\�����Ȃ�
	ShowCursor(false);

	// ���Z�b�g
	memset(state_.keyboard.prev_state_, 0x00, 256);
	memset(state_.keyboard.current_state_, 0x00, 256);

	return true;
}

void InputSystem::Update()
{
	// �L�[�{�[�h�̏�Ԃ��X�V
	memcpy(state_.keyboard.prev_state_, state_.keyboard.current_state_, 256);
	GetKeyboardState(state_.keyboard.current_state_);

	// �}�E�X�̏�Ԃ��X�V
	POINT current_pos;
	GetCursorPos(&current_pos);
	state_.mouse.position_ = DirectX::XMFLOAT2(current_pos.x, current_pos.y);

	// �����ɖ߂�
	//SetCursorPos(state_.mouse.center_.x, state_.mouse.center_.y);
}


