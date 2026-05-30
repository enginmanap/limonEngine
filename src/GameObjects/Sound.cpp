//
// Created by engin on 16.07.2018.
//

#include "Sound.h"
#include "../Assets/AssetManager.h"
#include "../ALHelper.h"
#include "../Assets/SoundAsset.h"
#include "../../libs/ImGui/imgui.h"

Sound::Sound(uint32_t worldID, std::shared_ptr<AssetManager> assetManager, const std::string &filename)
        : name(filename), worldID(worldID), assetManager(assetManager) {
    transformation.setUpdateCallback([this]() noexcept { onTransformUpdated(); });
}

void Sound::onTransformUpdated() noexcept {
    this->position = glm::vec3(transformation.getWorldTransform()[3]);
    if (soundHandleID != 0) {
        assetManager->getAlHelper()->setSourcePosition(soundHandleID, listenerRelative, position);
    }
}

void Sound::setLoop(bool looped) {
    this->looped = looped;
    if(this->soundHandleID != 0) {
        assetManager->getAlHelper()->setLooped(soundHandleID, this->looped);
    }
}

void Sound::setStartPosition(float startSecond) {
    this->startSecond = startSecond;
    std::cerr << "This method [setStartPosition] is not implemented yet " << std::endl;
}

void Sound::setStopPosition(float stopPosition) {
    this->stopPosition = stopPosition;
    std::cerr << "This method [setStopPosition] is not implemented yet " << std::endl;
}

void Sound::play() {
    if(playState == State::STOP_AFTER_FINISH) {
        if(this->looped && assetManager->getAlHelper()->isPlaying(soundHandleID)) {
            assetManager->getAlHelper()->setLooped(soundHandleID, this->looped);
            this->playState = State::PLAYING;
            return;
        } else {
            this->stop();//force stop if stop after finish
        }
    } else {
        if (soundHandleID != 0) {
            if (!assetManager->getAlHelper()->isPlaying(soundHandleID)) {//don't play if already playing
                soundHandleID = assetManager->getAlHelper()->play(assetManager->loadAsset<SoundAsset>({this->name}),
                                                                  this->looped, gain, referenceDistance, maxDistance);
                assetManager->getAlHelper()->setSourcePosition(soundHandleID, this->listenerRelative, this->position);
            }
        } else {
            soundHandleID = assetManager->getAlHelper()->play(assetManager->loadAsset<SoundAsset>({this->name}),
                                                              this->looped, gain, referenceDistance, maxDistance);
            assetManager->getAlHelper()->setSourcePosition(soundHandleID, this->listenerRelative, this->position);

        }
        this->playState = State::PLAYING;
    }
}

void Sound::stop() {
    assetManager->getAlHelper()->stop(soundHandleID);
    this->playState = State::STOPPED;
    soundHandleID = 0;
}

void Sound::pause() {
    assetManager->getAlHelper()->pause(soundHandleID);
    this->playState = State::PAUSED;
}

void Sound::resume() {
    assetManager->getAlHelper()->resume(soundHandleID);
    this->playState = State::PLAYING;
}

void Sound::stopAfterFinish() {
    if(soundHandleID != 0 && this->playState == State::PLAYING) {
        if (!assetManager->getAlHelper()->setLooped(soundHandleID, false)) {
            std::cerr << "The stop after finish is failed for " << this->name << "with handle " << soundHandleID <<  std::endl;
        }
    }
    this->playState = State::STOP_AFTER_FINISH;

}

void Sound::setWorldPosition(glm::vec3 position, bool listenerRelative) {
    this->position = position;
    this->listenerRelative = listenerRelative;
    if (parentObject == nullptr) {
        // Only drive the transformation when unattached — otherwise the parent controls world position.
        transformation.setTranslate(position);
    }
    if(this->soundHandleID != 0) {
        assetManager->getAlHelper()->setSourcePosition(soundHandleID, this->listenerRelative, this->position);
    }
}

Sound::~Sound() {
    if(soundHandleID != 0) {//we don't create asset until play, this check verifies it.
        this->stop();
        this->assetManager->freeAsset({this->name});
    } else {
        this->stop();
    }
}

Sound::State Sound::getState() {
    if(playState == State::STOP_AFTER_FINISH) {
        //check if stopped or not
        if(!assetManager->getAlHelper()->isPlaying(soundHandleID)) {
            playState = State::STOPPED;
        }
    }
    return playState;
}

bool Sound::changeGain(float gain) {
    this->gain = gain;
    if(soundHandleID != 0) {
        return assetManager->getAlHelper()->changeGain(this->soundHandleID, gain);
    } else {
        return false;
    }
}

ImGuiResult Sound::addImGuiEditorElements(const ImGuiRequest& request) {
    ImGuiResult result;
    if(transformation.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix, false, parentObject != nullptr)) {
        //true means transformation changed, activate rigid body
        result.updated = true;
    }
    ImGui::Text("File: %s", name.c_str());
    ImGui::NewLine();

    float gainEdit = gain;
    if (ImGui::DragFloat("Gain", &gainEdit, 1.0f, 0.0f, 5000.0f)) {
        changeGain(gainEdit);
        result.updated = true;
    }

    float referenceDistanceEdit = referenceDistance;
    if (ImGui::DragFloat("Reference Distance", &referenceDistanceEdit, 0.5f, 0.1f, 500.0f)) {
        referenceDistance = referenceDistanceEdit;
        result.updated = true;
    }

    float maxDistanceEdit = maxDistance;
    if (ImGui::DragFloat("Max Distance", &maxDistanceEdit, 1.0f, referenceDistance, 2000.0f)) {
        maxDistance = maxDistanceEdit;
        result.updated = true;
    }

    bool loopedEdit = looped;
    if (ImGui::Checkbox("Looped", &loopedEdit)) {
        setLoop(loopedEdit);
        result.updated = true;
    }

    bool autoPlayEdit = autoPlay;
    if (ImGui::Checkbox("Auto Play", &autoPlayEdit)) {
        autoPlay = autoPlayEdit;
        result.updated = true;
    }

    bool listenerRelativeEdit = listenerRelative;
    if (ImGui::Checkbox("Listener Relative (2D)", &listenerRelativeEdit)) {
        setWorldPosition(position, listenerRelativeEdit);
        result.updated = true;
    }

    if (!listenerRelative) {
        if (parentObject != nullptr) {
            ImGui::Text("World Position X: %.3f", position.x);
            ImGui::Text("World Position Y: %.3f", position.y);
            ImGui::Text("World Position Z: %.3f", position.z);
            ImGui::NewLine();
            glm::vec3 localPos = transformation.getTranslateSingle();
            bool localUpdated = false;
            localUpdated = ImGui::DragFloat("Local X", &localPos.x, 0.01f) || localUpdated;
            localUpdated = ImGui::DragFloat("Local Y", &localPos.y, 0.01f) || localUpdated;
            localUpdated = ImGui::DragFloat("Local Z", &localPos.z, 0.01f) || localUpdated;
            if (localUpdated) {
                transformation.setTranslate(localPos);
                result.updated = true;
            }
        } else {
            glm::vec3 pos = transformation.getTranslateSingle();
            bool posUpdated = false;
            posUpdated = ImGui::DragFloat("Position X", &pos.x, 0.01f) || posUpdated;
            posUpdated = ImGui::DragFloat("Position Y", &pos.y, 0.01f) || posUpdated;
            posUpdated = ImGui::DragFloat("Position Z", &pos.z, 0.01f) || posUpdated;
            if (posUpdated) {
                transformation.setTranslate(pos);
                result.updated = true;
            }
        }
    }

    const char* stateStr = "Unknown";
    switch (getState()) {
        case State::STOPPED:          stateStr = "Stopped"; break;
        case State::PLAYING:          stateStr = "Playing"; break;
        case State::PAUSED:           stateStr = "Paused"; break;
        case State::STOP_AFTER_FINISH: stateStr = "Stop After Finish"; break;
    }
    ImGui::NewLine();
    ImGui::Text("State: %s", stateStr);
    if (ImGui::Button("Play"))  { play();  result.updated = true; }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))  { stop();  result.updated = true; }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) { pause(); result.updated = true; }

    ImGui::NewLine();
    if (ImGui::Button("Remove This Sound")) {
        result.remove = true;
    }

    return result;
}
