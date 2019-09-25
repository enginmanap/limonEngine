//
// Created by engin on 28.07.2018.
//

#include "GUIButton.h"
#include "../GUI/GUILayer.h"
#include "../../libs/ImGui/imgui.h"
#include "../../libs/ImGuizmo/ImGuizmo.h"
#include "../Assets/AssetManager.h"
#include "../Assets/TextureAsset.h"
#include "TriggerObject.h"
#include "GamePlay/APISerializer.h"

GUIButton::GUIButton(uint32_t worldID, AssetManager *assetManager, LimonAPI *limonAPI, const std::string name,
                     const std::vector<std::string> &imageFiles)
        : GUIImageBase(
        assetManager->getGraphicsWrapper(), assetManager, imageFiles[0]), worldID(worldID), name(name), limonAPI(limonAPI) {
    this->imageFiles = imageFiles;
    this->images.push_back(this->image);
    strncpy(GUINameBuffer, this->name.c_str(), sizeof(GUINameBuffer) - 1);
    strncpy(GUIFileNameBuffer[0], this->imageFiles[0].c_str(), sizeof(GUIFileNameBuffer[0]));

    if(this->imageFiles.size() > 1) {
        strncpy(GUIFileNameBuffer[1], this->imageFiles[1].c_str(), sizeof(GUIFileNameBuffer[1]));
        this->images.push_back(assetManager->loadAsset<TextureAsset>({imageFiles[1]}));
        if (this->imageFiles.size() > 2) {
            strncpy(GUIFileNameBuffer[2], this->imageFiles[2].c_str(), sizeof(GUIFileNameBuffer[2]));
            this->images.push_back(assetManager->loadAsset<TextureAsset>({imageFiles[2]}));
            if (this->imageFiles.size() > 3) {
                strncpy(GUIFileNameBuffer[3], this->imageFiles[3].c_str(), sizeof(GUIFileNameBuffer[3]));
                this->images.push_back(assetManager->loadAsset<TextureAsset>({imageFiles[3]}));
            }
        }
    }
    this->setImageFromFlags();
}



GameObject::ObjectTypes GUIButton::getTypeID() const {
    return GUI_BUTTON;
}

std::string GUIButton::getName() const {
    return this->name;
}

void GUIButton::addedToLayer(GUILayer *layer) {
    parentLayers.push_back(layer);
}


GUIButton::~GUIButton() {
    for (size_t i = 0; i < parentLayers.size(); ++i) {
        parentLayers[i]->removeGuiElement(this->getWorldObjectID());
    }
}


bool GUIButton::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options) {
    tinyxml2::XMLElement *guiButtonNode = document.NewElement("GUIElement");
    parentNode->InsertEndChild(guiButtonNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("Type");
    currentElement->SetText("GUIButton");
    guiButtonNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(getWorldObjectID());
    guiButtonNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Name");
    currentElement->SetText(this->name.c_str());
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

    if(onClickTriggerCode != nullptr) {
        APISerializer::serializeTriggerCode(*onClickTriggerCode, document, guiButtonNode, "onClickTrigger", onClickParameters, enabled);
    }

    return true;
}


GUIButton *GUIButton::deserialize(tinyxml2::XMLElement *GUIRenderableNode, AssetManager *assetManager, Options *options,
                                  LimonAPI *limonAPI) {

    tinyxml2::XMLElement* GUIRenderableAttribute;

    GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Type");
    if (GUIRenderableAttribute == nullptr) {
        std::cerr << "GUI renderable must have a type. Skipping" << std::endl;
        return nullptr;
    }
    std::string type = GUIRenderableAttribute->GetText();
    if(type == "GUIButton") {
        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("ID");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI renderable must have a ID. Skipping" << std::endl;
            return nullptr;
        }
        uint32_t id = std::stoi(GUIRenderableAttribute->GetText());

        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Name");
        if (GUIRenderableAttribute == nullptr || GUIRenderableAttribute->GetText() == nullptr) {
            std::cerr << "GUI renderable must have a name. Skipping" << std::endl;
            return nullptr;
        }
        std::string name = GUIRenderableAttribute->GetText();

        std::vector<std::string> fileNames;
        for (uint32_t i = 0; i < 4; ++i) {
            //now get the file
            tinyxml2::XMLElement* fileAttribute = GUIRenderableNode->FirstChildElement(std::string("File-" + std::to_string(i)).c_str());
            if (fileAttribute == nullptr) {
                if(i == 0) {
                    std::cerr << "GUI Button must have at least one image file. Skipping" << std::endl;
                    return nullptr;
                } else {
                    //std::cout << "GUI Button creation will continue with only " << i << " images." << std::endl;
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
        GUIButton* element = new GUIButton(id, assetManager, limonAPI, name,
                                           fileNames);
        element->getTransformation()->setTranslate(tr.getTranslate());
        element->getTransformation()->setOrientation(tr.getOrientation());
        element->getTransformation()->setScale(tr.getScale());

        element->onClickTriggerCode = APISerializer::deserializeTriggerCode(GUIRenderableNode, GUIRenderableAttribute, "onClickTrigger", element->limonAPI,
                                                                         element->onClickParameters, element->enabled);
        if(element->onClickTriggerCode == nullptr) {
            std::cout << "Button On click trigger code deserialization failed." << std::endl;
        }
        element->setImageFromFlags();

        return element;
    }

    //unknown type case
    return nullptr;
}

GameObject::ImGuiResult GUIButton::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;

    //double # because I don't want to show it
    ImGui::InputText("Name##SelectedGUIButtonNameField", GUINameBuffer, sizeof(GUINameBuffer));
    this->name = GUINameBuffer;

    for (size_t i = 0; i < 4; ++i) {
        //double # because I don't want to show it
        ImGui::InputText(editorFileNameFields[i], GUIFileNameBuffer[i], sizeof(GUIFileNameBuffer));
        if (ImGui::Button(editorApplyFields[i])) {
            std::string enteredFileName = std::string(GUIFileNameBuffer[i]);
            TextureAsset *newAsset = assetManager->loadAsset<TextureAsset>({enteredFileName});
            if (newAsset != nullptr) {
                if(images[i] != nullptr) {
                    assetManager->freeAsset({images[i]->getName()});
                }
                this->images[i] = newAsset;
                this->setScale(newAsset->getHeight() / 2.0f,
                        newAsset->getWidth() / 2.0f); // split in half, because the quad is -1 to 1, meaning it is 2 units long.
                this->imageFiles[i] = enteredFileName;
            } else {
                std::cerr << "New image file can't be read" << std::endl;
            }
        }
    }
    result.updated = this->transformation.addImGuiEditorElements(request.ortogonalCameraMatrix, request.ortogonalMatrix, true);

    if (ImGui::CollapsingHeader("Click To Trigger")) {
        TriggerObject::PutTriggerInGui(limonAPI, this->onClickTriggerCode, this->onClickParameters, enabled, 0);
    }

    if (ImGui::Button("Remove")) {
        result.remove = true;
    }

    return result;
}
