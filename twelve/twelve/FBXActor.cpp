#include "FBXActor.h"

#include "FBXComponent.h"
#include "Model.h"

FBXActor::FBXActor(Scene* scene) : Actor(scene),
                                   model_(nullptr) {
}

FBXActor::~FBXActor() {
}

void FBXActor::UpdateActor(float delta_time) {
}

void FBXActor::SetFBXModel(const char* file_name) {
    model_ = new FBXComponent(this, file_name);
}

void FBXActor::SetNormalMap(const char* file_name) {
    if (model_ == nullptr) {
        return;
    }

    model_->CreateNormalMapAndView(file_name);
}

void FBXActor::SetArmMap(const char* file_name) {
    if (model_ == nullptr) {
        return;
    }

    model_->CreateArmMapAndView(file_name);
}
