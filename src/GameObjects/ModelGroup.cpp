//
// Created by engin on 26.11.2018.
//

#include "ModelGroup.h"
#include "Model.h"
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

    this->transformation.serialize(document, objectGroupNode);

    tinyxml2::XMLElement *countNode = document.NewElement("ChildCount");
    countNode->SetText(std::to_string(this->renderables.size()).c_str());
    objectGroupNode->InsertEndChild(countNode);

    tinyxml2::XMLElement *childrenNode = document.NewElement("Children");
    objectGroupNode->InsertEndChild(childrenNode);

    for (size_t i = 0; i < renderables.size(); ++i) {
        tinyxml2::XMLElement *currentElement = document.NewElement("Child");
        currentElement->SetAttribute("Index", std::to_string(i).c_str());
        childrenNode->InsertEndChild(currentElement);

        renderables[i]->fillObjects(document,currentElement);
    }

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

ModelGroup *ModelGroup::deserialize(GLHelper *glHelper, AssetManager *assetManager, tinyxml2::XMLElement *ModelGroupsNode,
                                    std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds,
                                    std::map<uint32_t, ModelGroup *> &childGroups,
                                    std::vector<std::unique_ptr<WorldLoader::ObjectInformation>> &childObjects, LimonAPI *limonAPI,
                                    ModelGroup *parentGroup) {
    ModelGroup* modelGroup = nullptr;

    tinyxml2::XMLElement* groupAttribute;
    std::string readName;
    uint32_t readID;
    groupAttribute = ModelGroupsNode->FirstChildElement("Name");
    if (groupAttribute == nullptr) {
        std::cerr << "Trigger must have a Name." << std::endl;
        return nullptr;
    }
    readName = groupAttribute->GetText();

    groupAttribute = ModelGroupsNode->FirstChildElement("ID");
    if (groupAttribute == nullptr) {
        std::cerr << "Trigger must have a ID." << std::endl;
        return nullptr;
    }
    readID = std::stoul(groupAttribute->GetText());

    modelGroup = new ModelGroup(glHelper, readID, readName);

    modelGroup->setParentObject(parentGroup);

    groupAttribute =  ModelGroupsNode->FirstChildElement("Transformation");
    if(groupAttribute == nullptr) {
        std::cerr << "Object does not have transformation. Can't be loaded" << std::endl;

        return nullptr;
    }
    modelGroup->transformation.deserialize(groupAttribute);
    if(parentGroup != nullptr) {
        modelGroup->getTransformation()->setParentTransform(parentGroup->getTransformation());
    }

    //now fill the children

    groupAttribute =  ModelGroupsNode->FirstChildElement("ChildCount");

    if(groupAttribute == nullptr) {
        std::cerr << "Model group " << modelGroup->getName() << " has no child count saved, no child will be loaded." << std::endl;
        return modelGroup;
    }

    uint32_t childCount = std::stoul(groupAttribute->GetText());

    modelGroup->renderables.resize(childCount);

    tinyxml2::XMLElement* childrenNode =  ModelGroupsNode->FirstChildElement("Children");

    if(childrenNode == nullptr) {
        std::cerr << "Children node not found for Model group " << modelGroup->getName() << " no child will be loaded." << std::endl;
        return modelGroup;
    }

    tinyxml2::XMLElement* childNode =  childrenNode->FirstChildElement("Child");

    while (childNode != nullptr) {
        if(childNode->Attribute("Index") == nullptr) {
            std::cerr << "child without ID found, this child of " << modelGroup->getName() << " will not be loaded. " << std::endl;
        } else {
            size_t childIndex = std::stoul(std::string(childNode->Attribute("Index")));

            if(childNode->FirstChildElement("Object")) {
                childObjects.push_back(WorldLoader::loadObject(assetManager,childNode->FirstChildElement("Object"), requiredSounds, limonAPI));
                Model* model = childObjects.at(childObjects.size()-1)->model;
                model->setParentObject(modelGroup);
                modelGroup->renderables[childIndex] = model;

                modelGroup->renderables[childIndex]->getTransformation()->setParentTransform(modelGroup->getTransformation());
            } else if(childNode->FirstChildElement("ObjectGroup")) {
                ModelGroup* newModelGroup = ModelGroup::deserialize(glHelper, assetManager,
                                                                    childNode->FirstChildElement("ObjectGroup"),
                                                                    requiredSounds, childGroups, childObjects, limonAPI,
                                                                    modelGroup);
                childGroups[newModelGroup->getWorldObjectID()] = newModelGroup;
                modelGroup->renderables[childIndex] = newModelGroup;
            } else {
                std::cerr << "The child is not Object, or Object group. Can't load unknown type, skipping." << std::endl;
            }
        }
        childNode =  childNode->NextSiblingElement("Child");
    }


    return modelGroup;
}
