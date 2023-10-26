#pragma once
#include <string>

#include "Actor.h"

class FBXActor : public Actor {
   public:
    FBXActor(class Scene* scene);
    ~FBXActor();
    void UpdateActor(float delta_time) override;

    void SetFBXModel(const char* file_name);
    void SetNormalMap(const char* file_name);
    void SetArmMap(const char* file_name);

   private:
    class FBXComponent* model_;
};