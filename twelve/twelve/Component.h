#pragma once
#include <cstdint>

class Component
{
public:
	Component(class Actor* owner, int update_order = 100);
	virtual ~Component();
	virtual void Update(float deltaTime);
	virtual void ProcessInput(const uint8_t* keyState) {}

	int GetUpdateOrder() const { return update_order_; }

private:
	class Actor* owner_;
	int update_order_;
};