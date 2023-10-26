#pragma once
#include <DirectXMath.h>

#include <cstdint>
#include <vector>

class Actor {
   public:
    enum State {
        EActive,
        EPaused,
        EDead
    };

    Actor(class Scene* scene);
    virtual ~Actor();

    void Update(float delta_time);
    void UpdateComponents(float delta_time);
    virtual void UpdateActor(float delta_time);

    void ProcessInput(const struct InputState& state);
    virtual void ActorInput(const struct InputState& state);

    void AddComponent(class Component* component);
    void RemoveComponent(class Component* component);

    const DirectX::XMFLOAT3& GetPosition() const { return position_; }
    void SetPosition(const DirectX::XMFLOAT3& pos) { position_ = pos; }
    float GetScale() const { return scale_; }
    void SetScale(float scale) { scale_ = scale; }
    const DirectX::XMFLOAT3& GetRotation() const { return rotation_; }
    void SetRotation(const DirectX::XMFLOAT3& rotation) { rotation_ = rotation; }

    DirectX::XMFLOAT3 GetForward() const { return forward_; }
    void SetForward(const DirectX::XMFLOAT3& forward) { forward_ = forward; }
    DirectX::XMFLOAT3 GetRight() const { return right_; }
    void SetRight(const DirectX::XMFLOAT3& right) { right_ = right; }

    State GetState() const { return state_; }
    void SetState(State state) { state_ = state; }

    class Scene* GetScene() const { return scene_; }

   protected:
    State state_;
    class Scene* scene_;

    float scale_;
    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT3 rotation_;
    DirectX::XMFLOAT3 forward_;
    DirectX::XMFLOAT3 right_;

    std::vector<class Component*> components_;
};
