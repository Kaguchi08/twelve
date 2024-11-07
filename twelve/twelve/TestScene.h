#pragma once
#include <unordered_map>

#include "FBXActor.h"
#include "PlaneActor.h"
#include "Scene.h"

class TestScene : public Scene {
   public:
    TestScene(class Game* game);
    ~TestScene();

    void ProcessInput(const struct InputState& state) override;

   private:
    std::vector<PlaneActor*> plane_actor_table_;
    std::vector<FBXActor*> fbx_actor_table;

    bool Initialize();
    void UpdateActor(float deltaTime) override;

    FBXActor* CreateFBXActor(const char* model, const char* normal, const char* arm,
                             const float scale = 1.0f,
                             const DirectX::XMFLOAT3& pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
                             const DirectX::XMFLOAT3& rot = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
    PlaneActor* CreatePlaneActor(const char* tex, const char* normal, const char* arm,
                                 const float scale = 1.0f,
                                 const DirectX::XMFLOAT3& pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
                                 const DirectX::XMFLOAT3& rot = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
};