//
// Created by engin on 16.07.2018.
//

#include "Sound.h"
#include "../Assets/AssetManager.h"
#include "../ALHelper.h"
#include "../Assets/SoundAsset.h"
#include "../../libs/ImGui/imgui.h"
#include "../XMLHelper.h"

Sound::Sound(uint32_t worldID, std::shared_ptr<AssetManager> assetManager, const std::string &filename)
        : name(filename), worldID(worldID), assetManager(assetManager) {
    transformation.setUpdateCallback([this]() noexcept { onTransformUpdated(); });
}

Sound::Sound(const Sound& other, uint32_t newObjectID)
        : name(other.name), worldID(newObjectID), assetManager(other.assetManager),
          position(other.position), listenerRelative(other.listenerRelative),
          startSecond(other.startSecond), stopPosition(other.stopPosition), gain(other.gain),
          referenceDistance(other.referenceDistance), maxDistance(other.maxDistance),
          looped(other.looped), autoPlay(other.autoPlay), temporary(other.temporary), channel(other.channel) {
    transformation.setUpdateCallback([this]() noexcept { onTransformUpdated(); });
    transformation.setTransformationsNotPropagate(
            other.transformation.getTranslate(), other.transformation.getOrientation(), other.transformation.getScale());
}

Attachable* Sound::clone(uint32_t newObjectID, LimonAPI* limonAPI [[gnu::unused]],
                         const std::unordered_map<uint32_t, uint32_t>& idRemap [[gnu::unused]]) const {
    return new Sound(*this, newObjectID);
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

void Sound::play(float fadeInSeconds) {
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
                                                                  this->looped, gain, referenceDistance, maxDistance, channel, fadeInSeconds);
                assetManager->getAlHelper()->setSourcePosition(soundHandleID, this->listenerRelative, this->position);
            }
        } else {
            soundHandleID = assetManager->getAlHelper()->play(assetManager->loadAsset<SoundAsset>({this->name}),
                                                              this->looped, gain, referenceDistance, maxDistance, channel, fadeInSeconds);
            assetManager->getAlHelper()->setSourcePosition(soundHandleID, this->listenerRelative, this->position);

        }
        this->playState = State::PLAYING;
    }
}

void Sound::fadeOutAndStop(float seconds) {
    if(soundHandleID != 0) {
        assetManager->getAlHelper()->fadeGain(soundHandleID, 0.0f, seconds, true);
    }
    this->playState = State::STOPPED;
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
    if (!listenerRelative) {
        if (parentObject != nullptr) {
            ImGui::Text("World Position X: %.3f", position.x);
            ImGui::Text("World Position Y: %.3f", position.y);
            ImGui::Text("World Position Z: %.3f", position.z);
            ImGui::NewLine();
        }
    }
    if(transformation.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix, false, parentObject != nullptr)) {
        //true means transformation changed, activate rigid body
        result.updated = true;
    }
    ImGui::Text("File: %s", name.c_str());
    ImGui::NewLine();

    float gainEdit = gain;
    if (ImGui::DragFloat("Gain", &gainEdit, 0.01f, 0.0f, 1.0f)) {
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

void Sound::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *soundsNode) const {
    if(temporary) {
        return;
    }

    tinyxml2::XMLElement* soundElement = document.NewElement("Sound");
    soundsNode->InsertEndChild(soundElement);

    XMLHelper::writeElement(document, soundElement, "File", name);
    XMLHelper::writeElement(document, soundElement, "ID", worldID);
    XMLHelper::writeElement(document, soundElement, "Gain", gain);
    XMLHelper::writeElement(document, soundElement, "ReferenceDistance", referenceDistance);
    XMLHelper::writeElement(document, soundElement, "MaxDistance", maxDistance);
    XMLHelper::writeElement(document, soundElement, "Looped", looped);
    XMLHelper::writeElement(document, soundElement, "AutoPlay", autoPlay);
    XMLHelper::writeElement(document, soundElement, "ListenerRelative", listenerRelative);

    if(getParentBoneID() != -1) {
        transformation.serializeLocal(document, soundElement);
    } else {
        transformation.serialize(document, soundElement);
    }

    if(getParentObject() != nullptr) {
        const GameObject* parentGO = dynamic_cast<const GameObject*>(getParentObject());
        if(parentGO != nullptr) {
            XMLHelper::writeElement(document, soundElement, "ParentID", parentGO->getWorldObjectID());
        }
        if(getParentBoneID() != -1) {
            XMLHelper::writeElement(document, soundElement, "ParentBoneID", getParentBoneID());
        }
    }
}

Sound *Sound::deserialize(tinyxml2::XMLElement *soundNode, std::shared_ptr<AssetManager> assetManager,
                           bool &hasParent, uint32_t &parentID, int32_t &parentBoneID) {
    hasParent = false;

    std::string filePath;
    if(!XMLHelper::readRequiredText(soundNode, "File", filePath, "Sound entry missing File element, skipping.")) {
        return nullptr;
    }

    std::string idStr;
    if(!XMLHelper::readRequiredText(soundNode, "ID", idStr, "Sound entry missing ID element, skipping.")) {
        return nullptr;
    }
    uint32_t soundID = std::stoul(idStr);

    Sound* sound = new Sound(soundID, assetManager, filePath);

    tinyxml2::XMLElement* gainEl = soundNode->FirstChildElement("Gain");
    if(gainEl != nullptr && gainEl->GetText() != nullptr) {
        //Gain is now normalized 0..1. Levels saved before normalization stored it on a 0..1000+ scale;
        //any value above the normalized max must be legacy, so migrate it on load.
        float storedGain = std::stof(gainEl->GetText());
        if(storedGain > 1.0f) {
            storedGain = storedGain / 1000.0f;
        }
        sound->changeGain(storedGain);
    }

    tinyxml2::XMLElement* refDistEl = soundNode->FirstChildElement("ReferenceDistance");
    if(refDistEl != nullptr && refDistEl->GetText() != nullptr) {
        sound->setReferenceDistance(std::stof(refDistEl->GetText()));
    }

    tinyxml2::XMLElement* maxDistEl = soundNode->FirstChildElement("MaxDistance");
    if(maxDistEl != nullptr && maxDistEl->GetText() != nullptr) {
        sound->setMaxDistance(std::stof(maxDistEl->GetText()));
    }

    tinyxml2::XMLElement* loopedEl = soundNode->FirstChildElement("Looped");
    if(loopedEl != nullptr && loopedEl->GetText() != nullptr) {
        sound->setLoop(std::string(loopedEl->GetText()) == "true");
    }

    tinyxml2::XMLElement* autoPlayEl = soundNode->FirstChildElement("AutoPlay");
    if(autoPlayEl != nullptr && autoPlayEl->GetText() != nullptr) {
        sound->setAutoPlay(std::string(autoPlayEl->GetText()) == "true");
    }

    tinyxml2::XMLElement* listenerRelEl = soundNode->FirstChildElement("ListenerRelative");
    bool listenerRelative = false;
    if(listenerRelEl != nullptr && listenerRelEl->GetText() != nullptr) {
        listenerRelative = std::string(listenerRelEl->GetText()) == "true";
    }

    tinyxml2::XMLElement* transformEl = soundNode->FirstChildElement("Transformation");
    if(transformEl != nullptr) {
        sound->getTransformation()->deserialize(transformEl);
    }

    glm::vec3 worldPos = glm::vec3(sound->getTransformation()->getWorldTransform()[3]);
    sound->setWorldPosition(worldPos, listenerRelative);

    tinyxml2::XMLElement* parentEl = soundNode->FirstChildElement("ParentID");
    if(parentEl != nullptr && parentEl->GetText() != nullptr) {
        parentID = std::stoul(parentEl->GetText());
        parentBoneID = -1;
        tinyxml2::XMLElement* parentBoneIDEl = soundNode->FirstChildElement("ParentBoneID");
        if(parentBoneIDEl != nullptr && parentBoneIDEl->GetText() != nullptr) {
            parentBoneID = std::stoi(parentBoneIDEl->GetText());
        }
        hasParent = true;
    }

    return sound;
}
