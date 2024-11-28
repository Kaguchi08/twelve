#pragma once
#include <DirectXMath.h>

#include <cstdint>
#include <vector>
#include <memory>

class Component;
class D3D12Wrapper;
struct InputState;

class Actor
{
public:
	Actor();
	virtual ~Actor();

	void Update(float deltaTime);
	void ProcessInput(const InputState& state);
	void GenerateOutput();

	void SetD3D12(std::shared_ptr<D3D12Wrapper> pD3D12);

	void AddComponent(std::shared_ptr<Component> pComponent);
	void RemoveComponent(std::shared_ptr<Component> pComponent);

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

protected:
	float scale_;
	DirectX::XMFLOAT3 position_;
	DirectX::XMFLOAT3 rotation_;
	DirectX::XMFLOAT3 forward_;
	DirectX::XMFLOAT3 right_;

	std::vector<std::shared_ptr<Component>> m_pComponents;

	virtual void UpdateActor(float deltaTime) = 0;
	virtual void ActorInput(const InputState& state) = 0;
};
