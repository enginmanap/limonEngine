//
// Created by engin on 18.06.2016.
//

#include <glm/ext.hpp>
#include "Light.h"
#include "../XMLHelper.h"


void Light::setPosition(glm::vec3 position, const Camera* playerCamera) {
    this->position = position;
    switch (lightType) {
        case LightTypes::NONE:
            return;
        case LightTypes::POINT:
            this->frustumChanged = true;
            static_cast<CubeCamera*>(cubeCameras[0])->getCameraMatrix();
            this->frustumChanged = true;
            break;
        case LightTypes::DIRECTIONAL:
            this->position = glm::normalize(position);
            updateLightView(playerCamera);
            break;
    }
    // Store as local offset when attached, world position otherwise.
    if(parentObject != nullptr) {
        const glm::mat4 parentInvWorld = glm::inverse(parentObject->getAttachmentTransformFor(parentBoneID)->getWorldTransform());
        attachTransformation.setTranslate(glm::vec3(parentInvWorld * glm::vec4(this->position, 1.0f)));
    } else {
        attachTransformation.setTranslate(this->position);
    }
}

ImGuiResult Light::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;

    ImGui::Text("Please note, Directional lights position setting is relative to player.");
    static glm::vec3 preciseTranslatePoint = this->position;
    bool crudeUpdated = false;
    bool positionAlreadySet = false;
    result.updated = ImGui::DragFloat("Color R", &(this->color.r), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::DragFloat("Color G", &(this->color.g), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::DragFloat("Color B", &(this->color.b), 0.0f, 1.0f)   || result.updated;
    ImGui::NewLine();
    bool attenuationUpdate = false;
    const bool isAttached = (parentObject != nullptr);
    switch (lightType) {
        case LightTypes::NONE:
            break;
        case LightTypes::POINT: {
            if(isAttached) {
                ImGui::Text("World Position X: %s", std::to_string(this->position.x).c_str());
                ImGui::Text("World Position Y: %s", std::to_string(this->position.y).c_str());
                ImGui::Text("World Position Z: %s", std::to_string(this->position.z).c_str());
                ImGui::NewLine();
                glm::vec3 localPos = attachTransformation.getTranslateSingle();
                bool localUpdated = false;
                localUpdated = ImGui::DragFloat("Local Position X", &(localPos.x), 0.01f, preciseTranslatePoint.x - 5.0f, preciseTranslatePoint.x + 5.0f) || localUpdated;
                localUpdated = ImGui::DragFloat("Local Position Y", &(localPos.y), 0.01f, preciseTranslatePoint.y - 5.0f, preciseTranslatePoint.y + 5.0f) || localUpdated;
                localUpdated = ImGui::DragFloat("Local Position Z", &(localPos.z), 0.01f, preciseTranslatePoint.z - 5.0f, preciseTranslatePoint.z + 5.0f) || localUpdated;
                ImGui::NewLine();
                crudeUpdated = ImGui::SliderFloat("Crude Local X", &(localPos.x), -100.0f, 100.0f) || crudeUpdated;
                crudeUpdated = ImGui::SliderFloat("Crude Local Y", &(localPos.y), -100.0f, 100.0f) || crudeUpdated;
                crudeUpdated = ImGui::SliderFloat("Crude Local Z", &(localPos.z), -100.0f, 100.0f) || crudeUpdated;
                if(localUpdated || crudeUpdated) {
                    const glm::mat4 parentWorld = parentObject->getAttachmentTransformFor(parentBoneID)->getWorldTransform();
                    this->setPosition(glm::vec3(parentWorld * glm::vec4(localPos, 1.0f)), request.playerCamera);
                    positionAlreadySet = true;
                    result.updated = true;
                }
                if(crudeUpdated) {
                    preciseTranslatePoint = localPos;
                }
            } else {
                result.updated = ImGui::DragFloat("Precise Position X", &(this->position.x), 0.01f, preciseTranslatePoint.x - 5.0f, preciseTranslatePoint.x + 5.0f) || result.updated;
                result.updated = ImGui::DragFloat("Precise Position Y", &(this->position.y), 0.01f, preciseTranslatePoint.y - 5.0f, preciseTranslatePoint.y + 5.0f) || result.updated;
                result.updated = ImGui::DragFloat("Precise Position Z", &(this->position.z), 0.01f, preciseTranslatePoint.z - 5.0f, preciseTranslatePoint.z + 5.0f) || result.updated;
                ImGui::NewLine();
                crudeUpdated = ImGui::SliderFloat("Crude Position X", &(this->position.x), -100.0f, 100.0f) || crudeUpdated;
                crudeUpdated = ImGui::SliderFloat("Crude Position Y", &(this->position.y), -100.0f, 100.0f) || crudeUpdated;
                crudeUpdated = ImGui::SliderFloat("Crude Position Z", &(this->position.z), -100.0f, 100.0f) || crudeUpdated;
            }
            ImGui::NewLine();
            attenuationUpdate = ImGui::DragFloat("Constant", &(this->attenuation.x), 0.01f, -10.0f, 1.0f) || attenuationUpdate;
            attenuationUpdate = ImGui::DragFloat("Linear", &(this->attenuation.y), 0.01f, 0.0f, 1.0f) || attenuationUpdate;
            attenuationUpdate = ImGui::DragFloat("Exponential", &(this->attenuation.z), 0.01f, 0.0f, 1.0f) || attenuationUpdate;
            if (attenuationUpdate) {
                this->setFrustumChanged(true);
                result.updated = true;
            }
            ImGui::NewLine();
        }
        break;
        case LightTypes::DIRECTIONAL: {
            result.updated = ImGui::DragFloat3("Ambient", glm::value_ptr(ambientColor), 0.01f, 0.0f, 1.0f) || result.updated;
            ImGui::NewLine();
            result.updated = ImGui::DragFloat("Precise Position X", &(this->position.x), 0.001, -1.0f, 1.0f)   || result.updated;
            result.updated = ImGui::DragFloat("Precise Position Y", &(this->position.y), 0.001, -1.0f, 0.0f)   || result.updated;
            result.updated = ImGui::DragFloat("Precise Position Z", &(this->position.z), 0.001, -1.0f, 1.0f)   || result.updated;
            this->position = glm::normalize(this->position);
        }
        break;
    }

    if((result.updated || crudeUpdated) && !positionAlreadySet) {
        this->setPosition(position, request.playerCamera);
    }
    if(crudeUpdated && !isAttached) {
        preciseTranslatePoint = this->position;
    }

    /* IMGUIZMO PART */

    static bool useSnap; //these are static because we want to keep the values
    static float snap[3] = {1.0f, 1.0f, 1.0f};
    ImGui::NewLine();
    ImGui::Checkbox("##SnapForLight", &(useSnap));
    ImGui::SameLine();
    ImGui::InputFloat3("Snap", &(snap[0]));

    glm::mat4 objectMatrix = isAttached ? attachTransformation.getWorldTransform()
                                        : glm::translate(glm::mat4(1.0f), position);
    ImGuizmo::BeginFrame();
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(glm::value_ptr(request.perspectiveCameraMatrix), glm::value_ptr(request.perspectiveMatrix), ImGuizmo::TRANSLATE, mCurrentGizmoMode, glm::value_ptr(objectMatrix), NULL, useSnap ? &(snap[0]) : NULL);

    const glm::vec3 newWorldPos(objectMatrix[3][0], objectMatrix[3][1], objectMatrix[3][2]);
    if(isAttached) {
        // Convert world-space gizmo result to local, then let the callback update this->position.
        const glm::mat4 parentInvWorld = glm::inverse(parentObject->getAttachmentTransformFor(parentBoneID)->getWorldTransform());
        attachTransformation.setTranslate(glm::vec3(parentInvWorld * glm::vec4(newWorldPos, 1.0f)));
    } else {
        this->setPosition(newWorldPos, request.playerCamera);
    }

    if(ImGui::Button("Remove light")) {
        result.remove = true;
        std::cout << "remove button press" << std::endl;
    }

    return result;
}

void Light::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *lightsNode) const {
    tinyxml2::XMLElement *lightElement = document.NewElement("Light");
    lightsNode->InsertEndChild(lightElement);

    const char* typeStr = "NONE";
    switch(lightType) {
        case LightTypes::NONE:        typeStr = "NONE";        break;
        case LightTypes::DIRECTIONAL: typeStr = "DIRECTIONAL"; break;
        case LightTypes::POINT:       typeStr = "POINT";       break;
    }
    XMLHelper::writeElement(document, lightElement, "Type", typeStr);
    XMLHelper::writeElement(document, lightElement, "ID", objectID);

    if(getParentBoneID() != -1) {
        attachTransformation.serializeLocal(document, lightElement);
    } else {
        attachTransformation.serialize(document, lightElement);
    }

    tinyxml2::XMLElement *colorNode = document.NewElement("Color");
    XMLHelper::writeElement(document, colorNode, "R", color.r);
    XMLHelper::writeElement(document, colorNode, "G", color.g);
    XMLHelper::writeElement(document, colorNode, "B", color.b);
    lightElement->InsertEndChild(colorNode);

    tinyxml2::XMLElement *attenuationNode = document.NewElement("Attenuation");
    XMLHelper::writeVec3(document, attenuationNode, attenuation);
    lightElement->InsertEndChild(attenuationNode);

    tinyxml2::XMLElement *ambientNode = document.NewElement("Ambient");
    XMLHelper::writeVec3(document, ambientNode, ambientColor);
    lightElement->InsertEndChild(ambientNode);

    if(getParentObject() != nullptr) {
        const GameObject* parentGO = dynamic_cast<const GameObject*>(getParentObject());
        if(parentGO != nullptr) {
            XMLHelper::writeElement(document, lightElement, "ParentID", parentGO->getWorldObjectID());
        }
        if(getParentBoneID() != -1) {
            XMLHelper::writeElement(document, lightElement, "ParentBoneID", getParentBoneID());
        }
    }
}

Light *Light::deserialize(tinyxml2::XMLElement *lightNode, GraphicsInterface *graphicsWrapper, uint32_t lightID,
                           bool &hasParent, uint32_t &parentID, int32_t &parentBoneID) {
    hasParent = false;

    std::string typeStr;
    if(!XMLHelper::readRequiredText(lightNode, "Type", typeStr, "Light must have a type.")) {
        return nullptr;
    }
    LightTypes type;
    if (typeStr == "POINT") {
        type = LightTypes::POINT;
    } else if (typeStr == "DIRECTIONAL") {
        type = LightTypes::DIRECTIONAL;
    } else {
        std::cerr << "Light type is not POINT or DIRECTIONAL. it is " << typeStr << std::endl;
        return nullptr;
    }

    glm::vec3 position;
    tinyxml2::XMLElement* lightTransformationElement = lightNode->FirstChildElement("Transformation");
    if(lightTransformationElement != nullptr) {
        tinyxml2::XMLElement* translateEl = lightTransformationElement->FirstChildElement("Translate");
        if(translateEl == nullptr || !XMLHelper::readVec3(translateEl, position)) {
            std::cerr << "Light Transformation is missing Translate." << std::endl;
            return nullptr;
        }
    } else {
        tinyxml2::XMLElement* positionEl = lightNode->FirstChildElement("Position");
        if (positionEl == nullptr) {
            std::cerr << "Light must have a position/direction." << std::endl;
            return nullptr;
        }
        if(!XMLHelper::readVec3(positionEl, position)) {
            return nullptr;
        }
    }

    glm::vec3 color;
    tinyxml2::XMLElement* colorEl = lightNode->FirstChildElement("Color");
    color.r = XMLHelper::readFloatOrDefault(colorEl, "R", 1.0f);
    color.g = XMLHelper::readFloatOrDefault(colorEl, "G", 1.0f);
    color.b = XMLHelper::readFloatOrDefault(colorEl, "B", 1.0f);

    Light* light = new Light(graphicsWrapper, lightID, type, position, color);

    if(lightTransformationElement != nullptr) {
        light->getTransformation()->deserialize(lightTransformationElement);
    }

    glm::vec3 attenuation(1, 0.1f, 0.01f);
    tinyxml2::XMLElement* attenuationEl = lightNode->FirstChildElement("Attenuation");
    if(attenuationEl != nullptr && XMLHelper::readVec3(attenuationEl, attenuation)) {
        light->setAttenuation(attenuation);
    }

    glm::vec3 ambientColor(1, 0.1f, 0.01f);
    tinyxml2::XMLElement* ambientEl = lightNode->FirstChildElement("Ambient");
    if(ambientEl != nullptr && XMLHelper::readVec3(ambientEl, ambientColor)) {
        light->setAmbientColor(ambientColor);
    }

    tinyxml2::XMLElement* parentEl = lightNode->FirstChildElement("ParentID");
    if(parentEl != nullptr && parentEl->GetText() != nullptr) {
        parentID = std::stoul(parentEl->GetText());
        parentBoneID = -1;
        tinyxml2::XMLElement* parentBoneIDEl = lightNode->FirstChildElement("ParentBoneID");
        if(parentBoneIDEl != nullptr && parentBoneIDEl->GetText() != nullptr) {
            parentBoneID = std::stoi(parentBoneIDEl->GetText());
        }
        hasParent = true;
    }

    return light;
}
