#include "PlaneActor.h"

#include "Dx12Wrapper.h"
#include "Game.h"
#include "PrimitiveComponent.h"
#include "Scene.h"

PlaneActor::PlaneActor(Scene* scene) : Actor(scene) {
    primitive_ = new PrimitiveComponent(this, PrimitiveType::kPlane);
}

PlaneActor::~PlaneActor() {
}

void PlaneActor::UpdateActor(float delta_time) {
}

void PlaneActor::SetTexture(const char* file_name) {
    primitive_->GetPrimitive()->CreateTextureAndView(scene_->GetGame()->GetDx12().get(), file_name);
}

void PlaneActor::SetNormalMap(const char* file_name) {
    primitive_->GetPrimitive()->CreateNormalMapAndView(scene_->GetGame()->GetDx12().get(), file_name);
}

void PlaneActor::SetArmMap(const char* file_name) {
    primitive_->GetPrimitive()->CreateArmMapAndView(scene_->GetGame()->GetDx12().get(), file_name);
}
