//
// Created by engin on 26.11.2018.
//

#include "ModelGroup.h"
#include "../../libs/imgui/imgui.h"

void ModelGroup::renderWithProgram(GLSLProgram &program) {
    for (auto renderable = renderables.begin(); renderable != renderables.end(); ++renderable) {
        (*renderable)->renderWithProgram(program);
    }
}

void ModelGroup::fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const {

    tinyxml2::XMLElement *objectGroupNode = document.NewElement("ObjectGroup");
    objectsNode->InsertEndChild(objectGroupNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("Name");
    currentElement->SetText(name.c_str());
    objectGroupNode->InsertEndChild(currentElement);

    for (auto renderable = renderables.begin(); renderable != renderables.end(); ++renderable) {
        (*renderable)->fillObjects(document, objectGroupNode);
    }
}

void ModelGroup::render() {
    for (auto renderable = renderables.begin(); renderable != renderables.end(); ++renderable) {
        (*renderable)->render();
    }
}

void ModelGroup::setupForTime(long time) {
    for (auto renderable = renderables.begin(); renderable != renderables.end(); ++renderable) {
        (*renderable)->setupForTime(time);
    }
}

GameObject::ObjectTypes ModelGroup::getTypeID() const {
    return MODEL_GROUP;
}

std::string ModelGroup::getName() const {
    return name;
}

uint32_t ModelGroup::getWorldObjectID() {
    return worldObjectID;
}

GameObject::ImGuiResult ModelGroup::addImGuiEditorElements(const GameObject::ImGuiRequest &request) {
    ImGuiResult result;
    if(this->renderables.size() == 0) {
        ImGui::Text("This group is empty");
    } else {
        //Allow transformation editing.
        if (transformation.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix)) {
            result.updated = true;
        }
    }
    return result;
}

const std::vector<PhysicalRenderable *> &ModelGroup::getRenderables() const {
    return renderables;
}
