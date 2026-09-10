//
// Created by engin on 18.06.2016.
//

#include <glm/ext.hpp>
#include "Light.h"
#include "../XMLHelper.h"
#include "../limonAPI/util/Logger.h"


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

// this is used to reorient precise drag inputs based on crude positions
static void clampToPreciseWindow(glm::vec3 &value, const glm::vec3 &center) {
    const float preciseDragWindow = 5.0f;
    value.x = glm::clamp(value.x, center.x - preciseDragWindow, center.x + preciseDragWindow);
    value.y = glm::clamp(value.y, center.y - preciseDragWindow, center.y + preciseDragWindow);
    value.z = glm::clamp(value.z, center.z - preciseDragWindow, center.z + preciseDragWindow);
}

ImGuiResult Light::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;

    static glm::vec3 preciseTranslatePoint = this->position;
    bool crudeUpdated = false;
    bool positionAlreadySet = false;
    const bool isAttached = (parentObject != nullptr);
    switch (lightType) {
        case LightTypes::NONE:
            break;
        case LightTypes::POINT: {
            ImGui::SeparatorText("Position");
            if(isAttached) {
                ImGui::Text("World: %.3f, %.3f, %.3f", this->position.x, this->position.y, this->position.z);
                glm::vec3 localPos = attachTransformation.getTranslateSingle();
                bool localUpdated = ImGui::DragFloat3("Precise Local", glm::value_ptr(localPos), 0.01f);
                if(localUpdated) {
                    clampToPreciseWindow(localPos, preciseTranslatePoint);
                }
                crudeUpdated = ImGui::SliderFloat3("Crude Local", glm::value_ptr(localPos), -100.0f, 100.0f);
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
                if(ImGui::DragFloat3("Precise Position", glm::value_ptr(this->position), 0.01f)) {
                    clampToPreciseWindow(this->position, preciseTranslatePoint);
                    result.updated = true;
                }
                crudeUpdated = ImGui::SliderFloat3("Crude Position", glm::value_ptr(this->position), -100.0f, 100.0f);
            }

            ImGui::SeparatorText("Light");
            result.updated = ImGui::ColorEdit3("Color", glm::value_ptr(this->color)) || result.updated;
            ImGui::SetItemTooltip("Hue only. Brightness is coming from Intensity.");
            result.updated = ImGui::DragFloat("Intensity", &(this->intensity), 0.05f, 0.0f, 100.0f) || result.updated;
            ImGui::SetItemTooltip("Brightness at the centre. Above 1 it saturates, widening the fully lit core.");
            float editedRadius = this->radius;
            if(ImGui::DragFloat("Radius", &editedRadius, 0.1f, 0.1f, 500.0f)) {
                this->setRadius(editedRadius);//Setting directly would break attenuation alignment
                result.updated = true;
            }
            ImGui::SetItemTooltip("Where the light reaches. Used by culling and shadows");

            ImGui::SeparatorText("Attenuation");
            float editedEdgeBrightness = this->edgeBrightness;
            if(ImGui::DragFloat("Edge Brightness", &editedEdgeBrightness, 0.005f, 0.01f, 1.0f)) {
                this->setEdgeBrightness(editedEdgeBrightness);
                result.updated = true;
            }
            ImGui::SetItemTooltip("Brightness at the radius, as fraction of center. 1 means as bright as center");
            //Limited at 1, as we don't want it to reverse the bightness. at 0 it would mean no light anywhere
            result.updated = ImGui::DragFloat("Falloff", &(this->falloffExponent), 0.1f, 1.0f, 32.0f) || result.updated;
            ImGui::SetItemTooltip("How fast the light loses its brightness close to center. Low values make it lose brightness faster.");
            //one shared budget, so dragging any of these moves the others in front of you instead of behind your back
            static const char* attenuationLabels[3] = {"Constant", "Linear", "Exponential"};
            for (int componentIndex = 0; componentIndex < 3; ++componentIndex) {
                float componentLimit = this->getAttenuationComponentLimit(componentIndex);
                float componentMinimum = (componentIndex == 0) ? 0.01f : 0.0f;//C is the divisor at distance zero
                float editedComponent = this->attenuation[componentIndex];
                if(ImGui::DragFloat(attenuationLabels[componentIndex], &editedComponent,
                                    glm::max(componentLimit / 200.0f, 0.0001f), componentMinimum, componentLimit)) {
                    this->setAttenuationComponent(componentIndex, editedComponent);
                    result.updated = true;
                }
            }
            ImGui::TextDisabled("Linear and Exponential needs to end up with radius. To ensure that, editing one auto calibrates the other.");
        }
        break;
        case LightTypes::DIRECTIONAL: {
            ImGui::SeparatorText("Direction");
            ImGui::TextDisabled("Relative to the player.");
            if(ImGui::DragFloat3("Precise Direction", glm::value_ptr(this->position), 0.001f, -1.0f, 1.0f)) {
                result.updated = true;
            }
            this->position.y = glm::min(this->position.y, 0.0f);//a directional light that points up lights nothing
            this->position = glm::normalize(this->position);

            ImGui::SeparatorText("Light");
            result.updated = ImGui::ColorEdit3("Color", glm::value_ptr(this->color)) || result.updated;
            result.updated = ImGui::ColorEdit3("Ambient", glm::value_ptr(ambientColor)) || result.updated;
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

void Light::renderLineVisualization(Logger *logger, uint32_t &bufferId) const {
    if(bufferId != 0) {
        logger->clearLineBuffer(bufferId);
        bufferId = 0;
    }
    if(lightType != LightTypes::POINT) {
        return;//Only point light has sphere visualization
    }
    const glm::vec3 activeDistanceColor(1.0f, 0.9f, 0.6f);//warm white, doesn't collide with the emitter palette
    bufferId = logger->drawSphere(getPosition(), getActiveDistance(), activeDistanceColor);
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

    XMLHelper::writeElement(document, lightElement, "Intensity", intensity);
    XMLHelper::writeElement(document, lightElement, "Radius", radius);
    XMLHelper::writeElement(document, lightElement, "Falloff", falloffExponent);
    XMLHelper::writeElement(document, lightElement, "EdgeBrightness", edgeBrightness);

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

    // We first assign the values that would calculate the attenuation alignment, then call setActiveDistance again
    // to trigger the alignment, otherwise it would work with invalid values
    light->intensity = XMLHelper::readFloatOrDefault(lightNode, "Intensity", 1.0f);
    light->radius = XMLHelper::readFloatOrDefault(lightNode, "Radius", 20.0f);
    light->falloffExponent = XMLHelper::readFloatOrDefault(lightNode, "Falloff", 4.0f);
    light->edgeBrightness = XMLHelper::readFloatOrDefault(lightNode, "EdgeBrightness", 0.5f);
    if(type == LightTypes::POINT) {
        static_cast<CubeCamera*>(light->cubeCameras[0])->setActiveDistance(light->radius);
    }

    glm::vec3 attenuation(1, 0.1f, 0.01f);
    tinyxml2::XMLElement* attenuationEl = lightNode->FirstChildElement("Attenuation");
    if(attenuationEl != nullptr && XMLHelper::readVec3(attenuationEl, attenuation)) {
        light->attenuation = attenuation;
    }
    // By calling this, we force the constant to be set, and other 2 to be realigned
    light->setAttenuationComponent(0, light->attenuation.x);//keeps Constant, pulls the other two onto the budget

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
