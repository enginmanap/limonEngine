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



