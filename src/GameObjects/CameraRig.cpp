//
// Created by engin on 2026.
//

#include "CameraRig.h"

#include <glm/gtx/matrix_decompose.hpp>
#include "ImGui/imgui.h"
#include "GamePlay/APISerializer.h"
#include "../XMLHelper.h"

CameraRig::CameraRig(const CameraRig& other, uint32_t newObjectID, LimonAPI* limonAPI) :
        CameraRig(newObjectID, other.name,
                  other.heldAttachment != nullptr ?
                          CameraExtensionInterface::createExtension(other.getRigTypeName(), limonAPI) : nullptr) {
    this->transformation.setTransformationsNotPropagate(
            other.transformation.getTranslate(),
            other.transformation.getOrientation(),
            other.transformation.getScale());
}

Attachable* CameraRig::clone(uint32_t newObjectID, LimonAPI* limonAPI,
                             const std::unordered_map<uint32_t, uint32_t>& idRemap) const {
    CameraRig* newRig = new CameraRig(*this, newObjectID, limonAPI);
    if (this->heldAttachment != nullptr && newRig->heldAttachment != nullptr) {
        std::vector<LimonTypes::GenericParameter> parameters = this->heldAttachment->getParameters();
        APISerializer::remapObjectReferenceParameters(parameters, idRemap);
        newRig->heldAttachment->setParameters(parameters);
    }
    return newRig;
}

ImGuiResult CameraRig::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;
    result.updated = transformation.addImGuiEditorElements(
        request.perspectiveCameraMatrix, request.perspectiveMatrix, false, getParentObject() != nullptr);
    if (ImGui::Button("Remove Camera Rig")) {
        result.remove = true;
    }
    return result;
}

void CameraRig::feedHeldAttachmentTransform() {
    if (heldAttachment == nullptr) {
        return;
    }
    Attachable* parent = getParentObject();
    if (parent == nullptr) {
        return; // unattached rig: the held behaviour produces its own pose
    }
    const glm::mat4 parentWorldTransform =
        parent->getAttachmentTransformFor(getParentBoneID())->getWorldTransform();
    // Decompose once here (C++) and feed components, so the held behaviour — including any Python rig —
    // never has to decompose a matrix every frame.
    glm::vec3 position, scale, skew;
    glm::quat orientation;
    glm::vec4 perspective;
    glm::decompose(parentWorldTransform, scale, orientation, position, skew, perspective);
    heldAttachment->setAttachmentTransform(position, orientation, scale);
}

void CameraRig::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *cameraRigsNode) const {
    tinyxml2::XMLElement *cameraRigNode = document.NewElement("CameraRig");
    cameraRigsNode->InsertEndChild(cameraRigNode);

    XMLHelper::writeElement(document, cameraRigNode, "Type", getRigTypeName());
    XMLHelper::writeElement(document, cameraRigNode, "ID", worldID);
    XMLHelper::writeElement(document, cameraRigNode, "Name", name);

    tinyxml2::XMLElement *parametersNode = document.NewElement("Parameters");
    std::vector<LimonTypes::GenericParameter> rigParameters =
        heldAttachment != nullptr ? heldAttachment->getParameters() : std::vector<LimonTypes::GenericParameter>();
    for (size_t i = 0; i < rigParameters.size(); ++i) {
        APISerializer::serializeParameterRequest(rigParameters[i], document, parametersNode, i);
    }
    cameraRigNode->InsertEndChild(parametersNode);

    if(getParentBoneID() != -1) {
        transformation.serializeLocal(document, cameraRigNode);
    } else {
        transformation.serialize(document, cameraRigNode);
    }

    if(getParentObject() != nullptr) {
        const GameObject* parentGO = dynamic_cast<const GameObject*>(getParentObject());
        if(parentGO != nullptr) {
            XMLHelper::writeElement(document, cameraRigNode, "ParentID", parentGO->getWorldObjectID());
        }
        if(getParentBoneID() != -1) {
            XMLHelper::writeElement(document, cameraRigNode, "ParentBoneID", getParentBoneID());
        }
    }
}

CameraRig *CameraRig::deserialize(tinyxml2::XMLElement *cameraRigNode, LimonAPI *limonAPI,
                                   bool &hasParent, uint32_t &parentID, int32_t &parentBoneID) {
    hasParent = false;

    std::string rigTypeName, idStr;
    if(!XMLHelper::readRequiredText(cameraRigNode, "Type", rigTypeName, "CameraRig entry missing Type or ID element, skipping.")
       || !XMLHelper::readRequiredText(cameraRigNode, "ID", idStr, "CameraRig entry missing Type or ID element, skipping.")) {
        return nullptr;
    }
    uint32_t rigID = std::stoul(idStr);

    CameraExtensionInterface* heldAttachment = CameraExtensionInterface::createExtension(rigTypeName, limonAPI);
    if(heldAttachment == nullptr) {
        std::cerr << "Camera rig type '" << rigTypeName << "' not found. Is the correct plugin loaded? Skipping." << std::endl;
        return nullptr;
    }

    std::vector<LimonTypes::GenericParameter> rigParameters;
    tinyxml2::XMLElement* parametersNode = cameraRigNode->FirstChildElement("Parameters");
    if(parametersNode != nullptr) {
        tinyxml2::XMLElement* parameterNode = parametersNode->FirstChildElement("Parameter");
        uint32_t index;
        while(parameterNode != nullptr) {
            std::shared_ptr<LimonTypes::GenericParameter> request = APISerializer::deserializeParameterRequest(parameterNode, index);
            if(request != nullptr && index <= rigParameters.size()) {
                rigParameters.insert(rigParameters.begin() + index, *request);
            }
            parameterNode = parameterNode->NextSiblingElement("Parameter");
        }
    }
    heldAttachment->setParameters(rigParameters);

    std::string rigName = rigTypeName + "_" + std::to_string(rigID);
    XMLHelper::readOptionalText(cameraRigNode, "Name", rigName);

    CameraRig* cameraRig = new CameraRig(rigID, rigName, heldAttachment);

    tinyxml2::XMLElement* transformEl = cameraRigNode->FirstChildElement("Transformation");
    if(transformEl != nullptr) {
        cameraRig->getTransformation()->deserialize(transformEl);
    }

    tinyxml2::XMLElement* parentEl = cameraRigNode->FirstChildElement("ParentID");
    if(parentEl != nullptr && parentEl->GetText() != nullptr) {
        parentID = std::stoul(parentEl->GetText());
        parentBoneID = -1;
        tinyxml2::XMLElement* parentBoneIDEl = cameraRigNode->FirstChildElement("ParentBoneID");
        if(parentBoneIDEl != nullptr && parentBoneIDEl->GetText() != nullptr) {
            parentBoneID = std::stoi(parentBoneIDEl->GetText());
        }
        hasParent = true;
    }

    return cameraRig;
}
