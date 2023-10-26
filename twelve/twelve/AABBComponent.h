#pragma once
#include "Collision.h"
#include "Component.h"

class AABBComponent : public Component {
   public:
    AABBComponent(class Actor* owner, int update_order = 100);
    ~AABBComponent();

    void UpdateCollision();

    void SetObjectBox(const AABB& model);
    void SetWorldBox(const AABB& world);

    const AABB& GetWorldBox() const { return world_box_; }
    const AABB& GetObjectBox() const { return object_box_; }

   private:
    AABB object_box_;
    AABB world_box_;
    bool should_rotate_;
};