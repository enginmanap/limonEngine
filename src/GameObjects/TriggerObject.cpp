//
// Created by engin on 8.05.2018.
//

#include "TriggerObject.h"
#include "../Assets/Animations/AnimationCustom.h"
#include "../../libs/ImGui/imgui.h"
#include "../GameObjects/Model.h"

void TriggerObject::render(BulletDebugDrawer *debugDrawer) {
    //render 12 lines

    glm::mat4 boxTransform = transformation.getWorldTransform();
    /* There are 8 points.
     * xyz
     * 1 +++
     * 2 ++-
     * 3 -++
     * 4 -+-
     *
     * 5 +-+
     * 6 +--
     * 7 --+
     * 8 ---
     * */

    //top
    debugDrawer->drawLine(boxTransform* glm::vec4( 1, 1, 1,1), boxTransform* glm::vec4( 1, 1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 2
    debugDrawer->drawLine(boxTransform* glm::vec4( 1, 1,-1,1), boxTransform* glm::vec4(-1, 1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 4
    debugDrawer->drawLine(boxTransform* glm::vec4(-1, 1,-1,1), boxTransform* glm::vec4(-1, 1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 3
    debugDrawer->drawLine(boxTransform* glm::vec4(-1, 1, 1,1), boxTransform* glm::vec4( 1, 1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 1

    //bottom
    debugDrawer->drawLine(boxTransform* glm::vec4( 1,-1, 1,1), boxTransform* glm::vec4( 1,-1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 2
    debugDrawer->drawLine(boxTransform* glm::vec4( 1,-1,-1,1), boxTransform* glm::vec4(-1,-1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 4
    debugDrawer->drawLine(boxTransform* glm::vec4(-1,-1,-1,1), boxTransform* glm::vec4(-1,-1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 3
    debugDrawer->drawLine(boxTransform* glm::vec4(-1,-1, 1,1), boxTransform* glm::vec4( 1,-1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 1

    //sides
    debugDrawer->drawLine(boxTransform* glm::vec4( 1, 1, 1,1), boxTransform* glm::vec4( 1,-1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 1
    debugDrawer->drawLine(boxTransform* glm::vec4( 1, 1,-1,1), boxTransform* glm::vec4( 1,-1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 2
    debugDrawer->drawLine(boxTransform* glm::vec4(-1, 1, 1,1), boxTransform* glm::vec4(-1,-1, 1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 3
    debugDrawer->drawLine(boxTransform* glm::vec4(-1, 1,-1,1), boxTransform* glm::vec4(-1,-1,-1,1), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 4
}

GameObject::ImGuiResult TriggerObject::addImGuiEditorElements(const glm::mat4 &cameraMatrix, const glm::mat4 &perspectiveMatrix) {
    static ImGuiResult request;

    transformation.addImGuiEditorElements(cameraMatrix, perspectiveMatrix);

    if (ImGui::CollapsingHeader("Trigger Properties")) {
        std::string currentTriggerName;
        if (this->triggerCode == nullptr) {
            currentTriggerName = "Not selected";
        } else {
            currentTriggerName = this->triggerCode->getName();
        }
        //let user select what kind of trigger required
        std::vector<std::string> triggerCodes = TriggerInterface::getTriggerNames();
        if (ImGui::BeginCombo("Trigger action type", currentTriggerName.c_str())) {
            for (auto it = triggerCodes.begin();
                 it != triggerCodes.end(); it++) {
                if (ImGui::Selectable(it->c_str())) {
                    if (this->triggerCode == nullptr ||
                        this->triggerCode->getName() != *it) {//ignore if same trigger selected

                        if (this->triggerCode != nullptr) {
                            delete this->triggerCode;
                        }
                        this->triggerCode = TriggerInterface::createTrigger(*it);
                        runParameters = triggerCode->getParameters();
                        this->enabled = false;
                    }
                }
            }
            ImGui::EndCombo();
        }
        if (this->triggerCode != nullptr) {
            bool isSet = LimonAPI::generateEditorElementsForParameters(runParameters);
            if (this->enabled) {
                if (ImGui::Button("Disable Trigger")) {
                    this->enabled = false;
                }
            } else {
                if (isSet) {
                    if (ImGui::Button("Enable Trigger")) {
                        this->enabled = true;
                    }
                } else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    ImGui::Button("Enable Trigger");
                    ImGui::PopStyleVar();
                }
            }


        }
    }
    return request;
}

void TriggerObject::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggersNode) const {

    tinyxml2::XMLElement *triggerNode= document.NewElement("Trigger");
    triggersNode->InsertEndChild(triggerNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("Name");
    currentElement->SetText(name.c_str());
    triggerNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(objectID);
    triggerNode->InsertEndChild(currentElement);

    if(triggerCode != nullptr) {
        currentElement = document.NewElement("TriggerCode");
        currentElement->SetText(triggerCode->getName().c_str());
        triggerNode->InsertEndChild(currentElement);
    }

    currentElement = document.NewElement("Enabled");
    if(this->enabled) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    triggerNode->InsertEndChild(currentElement);

    //now serialize the parameters

    currentElement = document.NewElement("RunParameters");
    for (size_t i = 0; i < runParameters.size(); ++i) {
        runParameters[i].serialize(document, currentElement, i);
    }
    triggerNode->InsertEndChild(currentElement);

    transformation.serialize(document, triggerNode);

}


bool TriggerObject::deserialize(tinyxml2::XMLElement *triggersNode) {

    tinyxml2::XMLElement* triggerAttribute;

    triggerAttribute = triggersNode->FirstChildElement("Name");
    if (triggerAttribute == nullptr) {
        std::cerr << "Trigger must have a Name." << std::endl;
        return false;
    }
    this->name = triggerAttribute->GetText();

    triggerAttribute = triggersNode->FirstChildElement("ID");
    if (triggerAttribute == nullptr) {
        std::cerr << "Trigger must have a ID." << std::endl;
        return false;
    }
    this->objectID = std::stoul(triggerAttribute->GetText());

    triggerAttribute = triggersNode->FirstChildElement("TriggerCode");
    if (triggerAttribute != nullptr) {
        this->triggerCode = TriggerInterface::createTrigger(triggerAttribute->GetText());
    }

    triggerAttribute = triggersNode->FirstChildElement("Enabled");
    if (triggerAttribute == nullptr) {
        std::cerr << "Trigger Didn't have enabled set, defaulting to False." << std::endl;
        this->enabled = false;
    } else {
        if(strcmp(triggerAttribute->GetText(), "True") == 0) {
            this->enabled = true;
        } else if(strcmp(triggerAttribute->GetText(), "False") == 0) {
            this->enabled = false;
        } else {
            std::cerr << "Trigger enabled setting is unknown value ["<< triggerAttribute->GetText()  <<"], can't be loaded " << std::endl;
            return false;
        }
    }

    triggerAttribute =  triggersNode->FirstChildElement("Transformation");
    if(triggerAttribute == nullptr) {
        std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;
        return false;
    }
    this->transformation.deserialize(triggerAttribute);


    triggerAttribute = triggersNode->FirstChildElement("RunParameters");

    tinyxml2::XMLElement* triggerParameter = triggerAttribute->FirstChildElement("Parameter");

    uint32_t index;
    while(triggerParameter != nullptr) {
        LimonAPI::ParameterRequest request;

        if(!request.deserialize(triggerParameter, index)) {
            return false;
        }
        runParameters.insert(runParameters.begin() + index, request);
        triggerParameter = triggerParameter->NextSiblingElement("Parameter");
    } // end of while (Trigger parameters)
    return true;
}


