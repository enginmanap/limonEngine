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
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4( 1, 1, 1,1)), glm::vec3(boxTransform* glm::vec4( 1, 1,-1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 2
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4( 1, 1,-1,1)), glm::vec3(boxTransform* glm::vec4(-1, 1,-1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 4
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4(-1, 1,-1,1)), glm::vec3(boxTransform* glm::vec4(-1, 1, 1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 3
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4(-1, 1, 1,1)), glm::vec3(boxTransform* glm::vec4( 1, 1, 1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 1

    //bottom
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4( 1,-1, 1,1)), glm::vec3(boxTransform* glm::vec4( 1,-1,-1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 2
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4( 1,-1,-1,1)), glm::vec3(boxTransform* glm::vec4(-1,-1,-1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 4
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4(-1,-1,-1,1)), glm::vec3(boxTransform* glm::vec4(-1,-1, 1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 3
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4(-1,-1, 1,1)), glm::vec3(boxTransform* glm::vec4( 1,-1, 1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 1

    //sides
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4( 1, 1, 1,1)), glm::vec3(boxTransform* glm::vec4( 1,-1, 1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 1 -> 1
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4( 1, 1,-1,1)), glm::vec3(boxTransform* glm::vec4( 1,-1,-1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 2 -> 2
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4(-1, 1, 1,1)), glm::vec3(boxTransform* glm::vec4(-1,-1, 1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 3 -> 3
    debugDrawer->drawLine(glm::vec3(boxTransform* glm::vec4(-1, 1,-1,1)), glm::vec3(boxTransform* glm::vec4(-1,-1,-1,1)), glm::vec3( 0, 0,1), glm::vec3( 0, 0,1), true);// 4 -> 4
}

GameObject::ImGuiResult TriggerObject::addImGuiEditorElements(const ImGuiRequest &request) {
    static ImGuiResult result;

    result.updated = transformation.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix);
    if (ImGui::CollapsingHeader("Trigger Properties")) {
        ImGui::Text("If first enter trigger is empty, enter trigger will be run for first time too.");
        if (ImGui::CollapsingHeader("First Enter Trigger")) {
            PutTriggerInGui(limonAPI, this->firstEnterTriggerCode, this->firstEnterParameters, enabledFirstTrigger, 0);
        }
        if (ImGui::CollapsingHeader("Enter Trigger")) {
            PutTriggerInGui(limonAPI, this->enterTriggerCode, this->enterParameters, enabledEnterTrigger, 1);
        }
        if (ImGui::CollapsingHeader("Exit Trigger")) {
            PutTriggerInGui(limonAPI, this->exitTriggerCode, this->exitParameters, enabledExitTrigger, 2);
        }
    }

    enabledAny = enabledFirstTrigger || enabledEnterTrigger || enabledExitTrigger;
    return result;
}

void TriggerObject::PutTriggerInGui(LimonAPI *limonAPI, TriggerInterface *&triggerCode, std::vector<LimonAPI::ParameterRequest> &parameters,
                                    bool &enabled, uint32_t index) {
    //index is used because imgui doesn't allow repeating labels
        //now we should put 3 triggers,
        std::string currentTriggerName;
        if (triggerCode == nullptr) {
            currentTriggerName = "Not selected";
        } else {
            currentTriggerName = triggerCode->getName();
        }
        //let user select what kind of trigger required
        std::vector<std::string> triggerCodes = TriggerInterface::getTriggerNames();

        if (ImGui::BeginCombo(("Trigger action type##" + std::to_string(index)).c_str(), currentTriggerName.c_str())) {
            for (auto it = triggerCodes.begin(); it != triggerCodes.end(); it++) {

                bool isThisCodeSelected =  (triggerCode != nullptr && triggerCode->getName() == *it);
                if (ImGui::Selectable(it->c_str(), isThisCodeSelected)) {
                    if (!isThisCodeSelected) {//if this is not the previously selected trigger code
                        if (triggerCode != nullptr) {
                            delete triggerCode;
                        }
                        triggerCode = TriggerInterface::createTrigger(*it, limonAPI);
                        parameters = triggerCode->getParameters();
                        enabled = false;
                    }
                }
                if(isThisCodeSelected) {
                    ImGui::SetItemDefaultFocus();
                }

            }
            ImGui::EndCombo();
        }
        if (triggerCode != nullptr) {
            bool isSet = limonAPI->generateEditorElementsForParameters(parameters, index);
            if (enabled) {
                if (ImGui::Button(("Disable Trigger##" + std::to_string(index)).c_str())) {
                    enabled = false;
                }
            } else {
                if (isSet) {
                    if (ImGui::Button(("Enable Trigger##" + std::to_string(index)).c_str())) {
                        enabled = true;
                    }
                } else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    ImGui::Button(("Enable Trigger##" + std::to_string(index)).c_str());
                    ImGui::PopStyleVar();
                }
            }


        }

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

    transformation.serialize(document, triggerNode);

    // There are 3 trigger codes, put them all
    serializeTriggerCode(document, triggerNode, firstEnterTriggerCode, "FirstEnterTriggerCode", firstEnterParameters);
    serializeTriggerCode(document, triggerNode, enterTriggerCode, "EnterTriggerCode", enterParameters);
    serializeTriggerCode(document, triggerNode, exitTriggerCode, "ExitTriggerCode", exitParameters);
}

void TriggerObject::serializeTriggerCode(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggerNode,
                                         TriggerInterface *triggerCode, const std::string &triggerCodeNodeName,
                                         const std::vector<LimonAPI::ParameterRequest> &parameters) const {
    if(triggerCode != nullptr) {
        tinyxml2::XMLElement *currentElement = document.NewElement(triggerCodeNodeName.c_str());

        tinyxml2::XMLElement* codeElement = document.NewElement("Name");
        codeElement->SetText(triggerCode->getName().c_str());
        currentElement->InsertEndChild(codeElement);

        //now serialize the parameters
        codeElement = document.NewElement("parameters");
        for (size_t i = 0; i < parameters.size(); ++i) {
            parameters[i].serialize(document, codeElement, i);
        }
        currentElement->InsertEndChild(codeElement);

        codeElement = document.NewElement("Enabled");
        if(enabledAny) {
            codeElement->SetText("True");
        } else {
            codeElement->SetText("False");
        }
        currentElement->InsertEndChild(codeElement);

        triggerNode->InsertEndChild(currentElement);
    }
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

    triggerAttribute =  triggersNode->FirstChildElement("Transformation");
    if(triggerAttribute == nullptr) {
        std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;
        return false;
    }
    this->transformation.deserialize(triggerAttribute);


    if(!deserializeTriggerCode(triggersNode, triggerAttribute, "FirstEnterTriggerCode", firstEnterTriggerCode, firstEnterParameters, enabledFirstTrigger)) {
        std::cerr << "First enter trigger code deserialization failed." << std::endl;
        return false;
    }

    if(!deserializeTriggerCode(triggersNode, triggerAttribute, "EnterTriggerCode", enterTriggerCode, enterParameters, enabledEnterTrigger)) {
        std::cerr << "enter trigger code deserialization failed." << std::endl;
        return false;
    }

    if(!deserializeTriggerCode(triggersNode, triggerAttribute, "ExitTriggerCode", exitTriggerCode, exitParameters, enabledExitTrigger)) {
        std::cerr << "Exit trigger code deserialization failed." << std::endl;
        return false;
    }

    enabledAny = enabledFirstTrigger ||enabledEnterTrigger || enabledExitTrigger;


    return true;
}

bool TriggerObject::deserializeTriggerCode(tinyxml2::XMLElement *triggersNode, tinyxml2::XMLElement *triggerAttribute,
                                           const std::string &nodeName, TriggerInterface *&triggerCode,
                                           std::vector<LimonAPI::ParameterRequest> &parameters, bool &enabled) const {
    enabled= false;
    triggerAttribute = triggersNode->FirstChildElement(nodeName.c_str());
    if (triggerAttribute != nullptr) {
        tinyxml2::XMLElement* triggerCodeAttribute = triggerAttribute->FirstChildElement("Name");
        triggerCode = TriggerInterface::createTrigger(triggerCodeAttribute->GetText(), limonAPI);

        triggerCodeAttribute = triggerAttribute->FirstChildElement("parameters");

        tinyxml2::XMLElement* triggerCodeParameter = triggerCodeAttribute->FirstChildElement("Parameter");

        uint32_t index;
        while(triggerCodeParameter != nullptr) {
            LimonAPI::ParameterRequest request;

            if(!request.deserialize(triggerCodeParameter, index)) {
                return false;
            }
            parameters.insert(parameters.begin() + index, request);
            triggerCodeParameter = triggerCodeParameter->NextSiblingElement("Parameter");
        } // end of while (Trigger parameters)

        triggerCodeAttribute = triggerAttribute->FirstChildElement("Enabled");
        if (triggerCodeAttribute == nullptr) {
            std::cerr << "Trigger Didn't have enabled set, defaulting to False." << std::endl;
            enabled = false;
        } else {
            if(strcmp(triggerCodeAttribute->GetText(), "True") == 0) {
                enabled = true;
            } else if(strcmp(triggerCodeAttribute->GetText(), "False") == 0) {
                enabled = false;
            } else {
                std::cerr << "Trigger enabled setting is unknown value [" << triggerCodeAttribute->GetText() << "], can't be loaded " << std::endl;
                return false;
            }
        }
    }
    return true;
}

std::vector<LimonAPI::ParameterRequest> TriggerObject::getResultOfCode(uint32_t codeID) {
    switch (codeID) {
        case 1: {
            if(firstEnterTriggerCode != nullptr) {
                return firstEnterTriggerCode->getResults();
            }
        } break;
        case 2: {
            if(enterTriggerCode != nullptr) {
                return enterTriggerCode->getResults();
            }
        } break;
        case 3: {
            if(exitTriggerCode != nullptr) {
                return exitTriggerCode->getResults();
            }
        } break;

    }
    return std::vector<LimonAPI::ParameterRequest>();
}


