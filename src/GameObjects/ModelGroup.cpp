//
// Created by engin on 26.11.2018.
//

#include "ModelGroup.h"
#include "../../libs/imgui/imgui.h"

void ModelGroup::renderWithProgram(GLSLProgram &program) {
    std::cerr << "Model Groups render with program used, it was not planned, nor tested." << std::endl;
    for (auto renderable = renderables.begin(); renderable != renderables.end(); ++renderable) {
        (*renderable)->renderWithProgram(program);
    }
}

void ModelGroup::fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const {

    tinyxml2::XMLElement *objectGroupNode = document.NewElement("ObjectGroup");
    objectsNode->InsertEndChild(objectGroupNode);

    tinyxml2::XMLElement *idNode = document.NewElement("ID");
    idNode->SetText(std::to_string(this->worldObjectID).c_str());
    objectGroupNode->InsertEndChild(idNode);

    tinyxml2::XMLElement *nameNode = document.NewElement("Name");
    nameNode->SetText(this->getName().c_str());
    objectGroupNode->InsertEndChild(nameNode);

    tinyxml2::XMLElement *countNode = document.NewElement("ChildCount");
    countNode->SetText(std::to_string(this->renderables.size()).c_str());
    objectGroupNode->InsertEndChild(countNode);

    tinyxml2::XMLElement *childrenNode = document.NewElement("Children");
    objectGroupNode->InsertEndChild(childrenNode);

    for (size_t i = 0; i < renderables.size(); ++i) {
        tinyxml2::XMLElement *currentElement = document.NewElement("Child");
        currentElement->SetAttribute("Index", std::to_string(i).c_str());
        GameObject* renderableObject = dynamic_cast<GameObject*>(renderables[i]);
        if(renderableObject != nullptr) {
            currentElement->SetText(std::to_string(renderableObject->getWorldObjectID()).c_str());
        } else {
            std::cerr << "the object group had renderable that is not game object. This case is not handled." << std::endl;
        }
        childrenNode->InsertEndChild(currentElement);
    }

    this->transformation.serialize(document, objectGroupNode);
}

void ModelGroup::render() {
    std::cerr << "Model Groups render used, it was not planned, nor tested." << std::endl;
    for (auto renderable = renderables.begin(); renderable != renderables.end(); ++renderable) {
        (*renderable)->render();
    }
}

void ModelGroup::setupForTime(long time) {
    std::cerr << "Model Groups setup for time used, it was not planned, nor tested." << std::endl;
    for (auto renderable = renderables.begin(); renderable != renderables.end(); ++renderable) {
        (*renderable)->setupForTime(time);
    }
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