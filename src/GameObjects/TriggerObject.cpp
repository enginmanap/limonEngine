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
    if(firstEnterTriggerCode != nullptr) {
        firstEnterTriggerCode->serializeTriggerCode(document, triggerNode, "FirstEnterTriggerCode",
                                                    firstEnterParameters, enabledFirstTrigger);
    }
    if(enterTriggerCode != nullptr) {
        enterTriggerCode->serializeTriggerCode(document, triggerNode, "EnterTriggerCode", enterParameters,
                                               enabledEnterTrigger);
    }
    if(exitTriggerCode != nullptr) {
        exitTriggerCode->serializeTriggerCode(document, triggerNode, "ExitTriggerCode", exitParameters,
                                              enabledExitTrigger);
    }
}




TriggerObject * TriggerObject::deserialize(tinyxml2::XMLElement *triggersNode, LimonAPI *limonAPI) {

    TriggerObject* triggerObject = new TriggerObject(0, limonAPI);//0 is place holder, deserialize sets real value;
    tinyxml2::XMLElement* triggerAttribute;

    triggerAttribute = triggersNode->FirstChildElement("Name");
    if (triggerAttribute == nullptr) {
        std::cerr << "Trigger must have a Name." << std::endl;
        delete triggerObject;
        return nullptr;
    }
    triggerObject->name = triggerAttribute->GetText();

    triggerAttribute = triggersNode->FirstChildElement("ID");
    if (triggerAttribute == nullptr) {
        std::cerr << "Trigger must have a ID." << std::endl;
        delete triggerObject;
        return nullptr;
    }
    triggerObject->objectID = std::stoul(triggerAttribute->GetText());

    triggerAttribute =  triggersNode->FirstChildElement("Transformation");
    if(triggerAttribute == nullptr) {
        std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;
        delete triggerObject;
        return nullptr;
    }
    triggerObject->transformation.deserialize(triggerAttribute);

    triggerObject->firstEnterTriggerCode = TriggerInterface::deserializeTriggerCode(triggersNode, triggerAttribute, "FirstEnterTriggerCode", triggerObject->limonAPI,
                                                                                    triggerObject->firstEnterParameters, triggerObject->enabledFirstTrigger);
    if(triggerObject->firstEnterTriggerCode == nullptr) {
        std::cout << "First enter trigger code deserialization failed." << std::endl;
    }
    triggerObject->enterTriggerCode = TriggerInterface::deserializeTriggerCode(triggersNode, triggerAttribute, "EnterTriggerCode", triggerObject->limonAPI,
                                                                               triggerObject->enterParameters, triggerObject->enabledEnterTrigger);
    if(triggerObject->enterTriggerCode == nullptr) {
        std::cout << "enter trigger code deserialization failed." << std::endl;
    }
    triggerObject->exitTriggerCode = TriggerInterface::deserializeTriggerCode(triggersNode, triggerAttribute, "ExitTriggerCode", triggerObject->limonAPI, triggerObject->exitParameters,
                                                                              triggerObject->enabledExitTrigger);
    if(triggerObject->exitTriggerCode == nullptr) {
        std::cout << "Exit trigger code deserialization failed." << std::endl;
    }

    triggerObject->enabledAny = triggerObject->enabledFirstTrigger ||triggerObject->enabledEnterTrigger || triggerObject->enabledExitTrigger;

    return triggerObject;
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


