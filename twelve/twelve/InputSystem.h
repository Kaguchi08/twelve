#pragma once
#include <DirectXMath.h>
#include <Windows.h>

#include "XMFLOAT_Helper.h"

enum ButtonState {
    kNone,
    kPressed,
    kReleased,
    kHeld
};

class KeyboardState {
   public:
    friend class InputSystem;

    ButtonState GetKeyState(BYTE keycode) const;
    bool GetKeyDown(BYTE keycode) const;

   private:
    BYTE prev_state_[256];
    BYTE current_state_[256];
};

class MouseState {
   public:
    friend class InputSystem;

    const DirectX::XMFLOAT2& GetPosition() const { return position_; }
    const DirectX::XMFLOAT2& GetDelta() const;
    const DirectX::XMFLOAT2& GetCenter() const { return center_; }

   private:
    DirectX::XMFLOAT2 position_;
    DirectX::XMFLOAT2 center_;
};

struct InputState {
    KeyboardState keyboard;
    MouseState mouse;
};

class InputSystem {
   public:
    InputSystem(class Game* game);

    bool Initialize();
    void Update();

    const InputState& GetState() const { return state_; }

   private:
    class Game* game_;
    InputState state_;
};
