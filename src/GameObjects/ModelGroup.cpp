//
// Created by engin on 26.11.2018.
//

#include "ModelGroup.h"
#include "Model.h"
#include "../../libs/ImGui/imgui.h"
#include <glm/gtx/matrix_decompose.hpp>

void ModelGroup::renderWithProgram(std::shared_ptr<GraphicsProgram> program, uint32_t lodLevel) {
    std::cerr << "Model Groups render with program used, it was not planned, nor tested." << std::endl;
    for (auto renderable = children.begin(); renderable != children.end(); ++renderable) {
        Renderable* r = dynamic_cast<Renderable*>(*renderable);
        if(r != nullptr) r->renderWithProgram(program, lodLevel);
    }
}

bool ModelGroup::fillObjects(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *objectsNode) const {

    tinyxml2::XMLElement *objectGroupNode = document.NewElement("ObjectGroup");
    objectsNode->InsertEndChild(objectGroupNode);

    tinyxml2::XMLElement *idNode = document.NewElement("ID");
    idNode->SetText(std::to_string(this->worldObjectID).c_str());
    objectGroupNode->InsertEndChild(idNode);

    tinyxml2::XMLElement *nameNode = document.NewElement("Name");
    nameNode->SetText(this->getName().c_str());
    objectGroupNode->InsertEndChild(nameNode);

    this->transformation.serialize(document, objectGroupNode);

    // Group children (models and nested groups) are serialized flat alongside every other object,
    // each carrying its own <ParentID>, and are reattached on load from that. The group therefore
    // stores only its own metadata here; nesting full children would duplicate them in the file.
    if(this->parentObject != nullptr) {
        GameObject* parent = dynamic_cast<GameObject*>(this->parentObject);
        if(parent != nullptr) {
            tinyxml2::XMLElement *parentIDNode = document.NewElement("ParentID");
            parentIDNode->SetText(std::to_string(parent->getWorldObjectID()).c_str());
            objectGroupNode->InsertEndChild(parentIDNode);
        }
    }
    return true;
}

void ModelGroup::setupForTime(long time) {
    std::cerr << "Model Groups setup for time used, it was not planned, nor tested." << std::endl;
    for (auto renderable = children.begin(); renderable != children.end(); ++renderable) {
        Renderable* r = dynamic_cast<Renderable*>(*renderable);
        if(r != nullptr) r->setupForTime(time);
    }
}

ImGuiResult ModelGroup::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;
    if(this->children.size() == 0) {
        ImGui::Text("This group is empty");
    } else {
        //Allow transformation editing.
        if (transformation.addImGuiEditorElements(request.perspectiveCameraMatrix, request.perspectiveMatrix, false, parentObject != nullptr)) {
            result.updated = true;
        }
    }
    return result;
}

ModelGroup *ModelGroup::deserialize(GraphicsInterface* graphicsWrapper, tinyxml2::XMLElement *ModelGroupsNode,
                                    uint32_t &parentIDOut) {
    parentIDOut = 0;//0 means no parent; world IDs start at 2 (1 is the player).

    tinyxml2::XMLElement* groupAttribute = ModelGroupsNode->FirstChildElement("Name");
    if (groupAttribute == nullptr || groupAttribute->GetText() == nullptr) {
        std::cerr << "Model group must have a Name." << std::endl;
        return nullptr;
    }
    std::string readName = groupAttribute->GetText();

    groupAttribute = ModelGroupsNode->FirstChildElement("ID");
    if (groupAttribute == nullptr || groupAttribute->GetText() == nullptr) {
        std::cerr << "Model group must have an ID." << std::endl;
        return nullptr;
    }
    uint32_t readID = std::stoul(groupAttribute->GetText());

    ModelGroup* modelGroup = new ModelGroup(graphicsWrapper, readID, readName);

    groupAttribute = ModelGroupsNode->FirstChildElement("Transformation");
    if(groupAttribute == nullptr) {
        std::cerr << "Model group does not have transformation. Can't be loaded" << std::endl;
        delete modelGroup;
        return nullptr;
    }
    modelGroup->transformation.deserialize(groupAttribute);

    // Children are loaded flat from <Objects>/<ObjectGroups> and reattached via their own <ParentID>.
    tinyxml2::XMLElement* parentIDElement = ModelGroupsNode->FirstChildElement("ParentID");
    if(parentIDElement != nullptr && parentIDElement->GetText() != nullptr) {
        parentIDOut = std::stoul(parentIDElement->GetText());
    }

    return modelGroup;
}

ModelGroup *ModelGroup::deserializeV1(GraphicsInterface* graphicsWrapper, std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *ModelGroupsNode,
                                    std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds,
                                    std::map<uint32_t, ModelGroup *> &childGroups,
                                    std::vector<std::unique_ptr<WorldLoader::ObjectInformation>> &childObjects, LimonAPI *limonAPI,
                                    ModelGroup *parentGroup) {
    // Old loader: group children are saved inside <Children>. to allow old world to load, we still support it.
    // V2 saves children flat (see deserialize) and reattaches them via addChild. That is the correct patch.
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

    modelGroup = new ModelGroup(graphicsWrapper, readID, readName);

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

    modelGroup->children.resize(childCount);

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
                std::vector<std::unique_ptr<WorldLoader::ObjectInformation>> childVector =
                WorldLoader::loadObject(assetManager, childNode->FirstChildElement("Object"), requiredSounds,
                                        limonAPI, modelGroup);

                childObjects.push_back(std::move(childVector[childVector.size() -1]));
                Model* model = childObjects.at(childObjects.size()-1)->model;
                modelGroup->children[childIndex] = model;

                modelGroup->children[childIndex]->getTransformation()->setParentTransform(modelGroup->getTransformation());
            } else if(childNode->FirstChildElement("ObjectGroup")) {
                ModelGroup* newModelGroup = ModelGroup::deserializeV1(graphicsWrapper, assetManager,
                                                                    childNode->FirstChildElement("ObjectGroup"),
                                                                    requiredSounds, childGroups, childObjects, limonAPI,
                                                                    modelGroup);
                childGroups[newModelGroup->getWorldObjectID()] = newModelGroup;
                modelGroup->children[childIndex] = newModelGroup;
            } else {
                std::cerr << "The child is not Object, or Object group. Can't load unknown type, skipping." << std::endl;
            }
        }
        childNode =  childNode->NextSiblingElement("Child");
    }


    return modelGroup;
}

void ModelGroup::addChild(PhysicalRenderable *renderable) {
    glm::vec3 averageTranslateDifference(0.0f, 0.0f, 0.0f);
    if(!children.empty()) {
        for (auto & iterator : children) {
            averageTranslateDifference += iterator->getTransformation()->getTranslateSingle();//single because we are the parent
        }
    }

    averageTranslateDifference += renderable->getTransformation()->getTranslateSingle() - this->transformation.getTranslate();//if already had parent, don't use it

    averageTranslateDifference = averageTranslateDifference * (1.0f / (children.size()+1)); //+1 because new renderable is not put in yet
    //at this point, we know where the model group imguizmo should be, now move it, and update old children

    glm::vec3 difference = averageTranslateDifference;
    for (auto iterator = children.begin(); iterator != children.end(); ++iterator) {
        (*iterator)->getTransformation()->setTransformationsNotPropagate((*iterator)->getTransformation()->getTranslateSingle() - difference);
    }

    this->getTransformation()->addTranslate(averageTranslateDifference);
    glm::vec3 translate, scale;
    glm::quat orientation;
    this->transformation.getDifferenceAddition(*renderable->getTransformation(), translate, scale, orientation);

    translate = translate * (1.0f / this->transformation.getScale());
    translate = translate * this->transformation.getOrientation();

    renderable->getTransformation()->setTranslate(translate);
    renderable->getTransformation()->setScale(scale);
    renderable->getTransformation()->setOrientation(orientation);
    renderable->getTransformation()->setParentTransform(&this->transformation);
    renderable->setParentObject(this);

    children.push_back(renderable);
}
