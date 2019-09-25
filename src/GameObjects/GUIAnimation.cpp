//
// Created by engin on 28.10.2018.
//

#include "GUIAnimation.h"
#include "../GUI/GUILayer.h"
#include "../../libs/ImGui/imgui.h"
#include "../../libs/ImGuizmo/ImGuizmo.h"
#include "../Assets/AssetManager.h"
#include "../Assets/TextureAsset.h"


GUIAnimation::GUIAnimation(uint32_t worldID, AssetManager *assetManager, const std::string name,
                           const std::vector<std::string> &imageFiles, long creationTime, uint32_t frameSpeed,
                           bool isLooped)
        : GUIImageBase(
        assetManager->getGraphicsWrapper(), assetManager, imageFiles[0]), worldID(worldID), name(name), creationTime(creationTime), imagePerFrame(frameSpeed), looped(isLooped) {
    this->imageFiles = imageFiles;
    this->images.push_back(this->image);//the first element is generated and held by base

    strncpy(GUINameBuffer, this->name.c_str(), sizeof(GUINameBuffer) - 1);

    //now copy all the images as they should
    this->imageFiles = imageFiles;

    for (size_t i = 1; i < this->imageFiles.size(); ++i) {//starts from 1 because 0 is handled by base
        this->images.push_back(assetManager->loadAsset<TextureAsset>({imageFiles[i]}));
    }
    this->duration = this->imagePerFrame * this->images.size();
}



GameObject::ObjectTypes GUIAnimation::getTypeID() const {
    return GUI_ANIMATION;
}

std::string GUIAnimation::getName() const {
    return this->name;
}

void GUIAnimation::addedToLayer(GUILayer *layer) {
    parentLayers.push_back(layer);
}


GUIAnimation::~GUIAnimation() {
    for (size_t i = 0; i < parentLayers.size(); ++i) {
        parentLayers[i]->removeGuiElement(this->getWorldObjectID());
    }
}


bool GUIAnimation::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options) {
    tinyxml2::XMLElement *guiButtonNode = document.NewElement("GUIElement");
    parentNode->InsertEndChild(guiButtonNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("Type");
    currentElement->SetText("GUIAnimation");
    guiButtonNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(getWorldObjectID());
    guiButtonNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Name");
    currentElement->SetText(this->name.c_str());
    guiButtonNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("CreationTime");
    currentElement->SetText(std::to_string(creationTime).c_str());
    guiButtonNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ImagePerFrame");
    currentElement->SetText(std::to_string(imagePerFrame).c_str());
    guiButtonNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Looped");
    if(this->looped) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    guiButtonNode->InsertEndChild(currentElement);

    for (size_t i = 0; i < imageFiles.size(); ++i) {
        currentElement = document.NewElement(std::string("File-" + std::to_string(i)).c_str());
        currentElement->SetText(this->imageFiles[i].c_str());
        guiButtonNode->InsertEndChild(currentElement);
    }

    Transformation temp(transformation);//this is to save relative, so resolution change won't effect position

    temp.setTranslate(glm::vec3(
            temp.getTranslate().x / options->getScreenWidth(),
            temp.getTranslate().y / options->getScreenHeight(),
            temp.getTranslate().z
    ));

    temp.setScale(glm::vec3(
            temp.getScale().x / options->getScreenWidth(),
            temp.getScale().y / options->getScreenHeight(),
            temp.getScale().z
    ));

    temp.serialize(document, guiButtonNode);

    return true;
}


GUIAnimation *GUIAnimation::deserialize(tinyxml2::XMLElement *GUIRenderableNode, AssetManager *assetManager, Options *options) {

    tinyxml2::XMLElement* GUIRenderableAttribute;

    GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Type");
    if (GUIRenderableAttribute == nullptr) {
        std::cerr << "GUI renderable must have a type. Skipping" << std::endl;
        return nullptr;
    }
    std::string type = GUIRenderableAttribute->GetText();
    if(type == "GUIAnimation") {
        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("ID");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI renderable must have a ID. Skipping" << std::endl;
            return nullptr;
        }
        uint32_t id = std::stoi(GUIRenderableAttribute->GetText());

        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Name");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI renderable must have a name. Skipping" << std::endl;
            return nullptr;
        }

        std::string name = GUIRenderableAttribute->GetText();

        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("CreationTime");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI renderable must have a Creation time. Skipping" << std::endl;
            return nullptr;
        }

        long creationTime = std::stol(GUIRenderableAttribute->GetText());

        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("ImagePerFrame");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI renderable must have frame speed. Skipping" << std::endl;
            return nullptr;
        }

        uint32_t imagePerFrame = std::stoul(GUIRenderableAttribute->GetText());

        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Looped");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI renderable must have is looped set. Skipping" << std::endl;
            return nullptr;
        }

        std::string loopedString = GUIRenderableAttribute->GetText();
        bool isLooped = false;
        if(loopedString == "True") {
            isLooped = true;
        }

        std::vector<std::string> fileNames;

        for (uint32_t i = 0; true; ++i) { //FIXME this is true because it should have been while, but childElementName prevents it
            //now get the file
            tinyxml2::XMLElement* fileAttribute = GUIRenderableNode->FirstChildElement(std::string("File-" + std::to_string(i)).c_str());
            if (fileAttribute == nullptr) {
                if(i == 0) {
                    std::cerr << "GUI Button must have at least one image file. Skipping" << std::endl;
                    return nullptr;
                } else {
                    std::cout << "GUI Image creation will continue with only " << i << " images." << std::endl;
                    break;
                }
            }
            fileNames.push_back(std::string(fileAttribute->GetText()));
        }
        GUIRenderableAttribute =  GUIRenderableNode->FirstChildElement("Transformation");
        if(GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI Button does not have transformation. Skipping" << std::endl;
            return nullptr;
        }
        Transformation tr;
        tr.deserialize(GUIRenderableAttribute);

        tr.setTranslate(glm::vec3(
                tr.getTranslate().x * options->getScreenWidth(),
                tr.getTranslate().y * options->getScreenHeight(),
                tr.getTranslate().z
        ));

        tr.setScale(glm::vec3(
                tr.getScale().x * options->getScreenWidth(),
                tr.getScale().y * options->getScreenHeight(),
                tr.getScale().z
        ));

        //now we have everything, create the GUI Button
        GUIAnimation* element = new GUIAnimation(id, assetManager, name,
                                                 fileNames, creationTime, imagePerFrame, isLooped);
        element->getTransformation()->setTranslate(tr.getTranslate());
        element->getTransformation()->setOrientation(tr.getOrientation());
        element->getTransformation()->setScale(tr.getScale());

        return element;
    }

    //unknown type case
    return nullptr;
}

GameObject::ImGuiResult GUIAnimation::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;

    //double # because I don't want to show it
    ImGui::InputText("Name##SelectedGUIButtonNameField", GUINameBuffer, sizeof(GUINameBuffer));
    this->name = GUINameBuffer;

    result.updated = this->transformation.addImGuiEditorElements(request.ortogonalCameraMatrix, request.ortogonalMatrix, true);

    if (ImGui::Button("Remove")) {
        result.remove = true;
    }

    return result;
}

void GUIAnimation::setupForTime(long time) {
    float currentTime = (time - creationTime) / ((float)1000 / (float)imagePerFrame);
    if(currentTime < 0) {
        currentTime = 0;//why is this? Because it handles if
        creationTime = time;
    }
    uint32_t currentElement;
    if(looped) {
        currentElement = static_cast<uint32_t >(std::floor(currentTime)) % this->images.size();
    } else {
        if(currentTime >= this->duration) {
            currentElement = this->images.size()-1;
        } else {
            currentElement = static_cast<uint32_t >(std::floor(currentTime));
        }
    }

    this->image = this->images[currentElement];
}
