//
// Created by engin on 26.07.2018.
//

#include "GUIImage.h"
#include "../GUI/GUILayer.h"
#include "GameObject.h"
#include "../../libs/ImGui/imgui.h"
#include "../../libs/ImGuizmo/ImGuizmo.h"
#include "../Assets/AssetManager.h"
#include "../Assets/TextureAsset.h"


GUIImage::GUIImage(uint32_t worldID, Options *options, AssetManager *assetManager, const std::string name,
    const std::string &imageFile)
    : GUIImageBase(
            assetManager->getGlHelper(), assetManager, imageFile), worldID(worldID), name(name), options(options) {
        strncpy(GUINameBuffer, this->name.c_str(), sizeof(GUINameBuffer) - 1);
        strncpy(GUIFileNameBuffer, this->imageFile.c_str(), sizeof(GUIFileNameBuffer));
    }


GameObject::ObjectTypes GUIImage::getTypeID() const {
    return GUI_IMAGE;
}

std::string GUIImage::getName() const {
    return this->name + "_" + std::to_string(this->getWorldObjectID());
}

void GUIImage::addedToLayer(GUILayer *layer) {
    parentLayers.push_back(layer);
}


GUIImage::~GUIImage() {
    for (size_t i = 0; i < parentLayers.size(); ++i) {
        parentLayers[i]->removeGuiElement(this->getWorldObjectID());
    }
}


bool GUIImage::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options) {
    tinyxml2::XMLElement *guiImageNode = document.NewElement("GUIElement");
    parentNode->InsertEndChild(guiImageNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("Type");
    currentElement->SetText("GUIImage");
    guiImageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(getWorldObjectID());
    guiImageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Name");
    currentElement->SetText(this->name.c_str());
    guiImageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("File");
    currentElement->SetText(this->imageFile.c_str());
    guiImageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("FullScreen");
    if(this->fullScreen) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    guiImageNode->InsertEndChild(currentElement);


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

    temp.serialize(document, guiImageNode);

    return true;
}


GUIImage *GUIImage::deserialize(tinyxml2::XMLElement *GUIRenderableNode, AssetManager *assetManager, Options *options) {

    tinyxml2::XMLElement* GUIRenderableAttribute;

    GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Type");
    if (GUIRenderableAttribute == nullptr) {
        std::cerr << "GUI renderable must have a type. Skipping" << std::endl;
        return nullptr;
    }
    std::string type = GUIRenderableAttribute->GetText();
    if(type == "GUIImage") {
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

        //now get the file
        tinyxml2::XMLElement* fileAttribute = GUIRenderableNode->FirstChildElement("File");
        if (fileAttribute == nullptr) {
            std::cerr << "GUI Image must have a file. Skipping" << std::endl;
            return nullptr;
        }
        std::string fileName = fileAttribute->GetText();

        bool fullScreen = false;
        tinyxml2::XMLElement* fullScreenAttribute =  GUIRenderableNode->FirstChildElement("FullScreen");
        if (fullScreenAttribute == nullptr) {
#ifndef NDEBUG
            std::cout << "GUI Image full screen setting not found. defaulting to False" << std::endl;
#endif
        } else {
            std::string fullScreenText = fullScreenAttribute->GetText();
            if(fullScreenText == "True") {
                fullScreen = true;
            } else if(fullScreenText == "False") {
                fullScreen = false;
            } else {
                std::cout << "Object disconnect status is unknown. defaulting to False" << std::endl;
            }
        }

        GUIRenderableAttribute =  GUIRenderableNode->FirstChildElement("Transformation");
        if(GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI Image does not have transformation. Skipping" << std::endl;
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
        //now we have everything, create the GUI Image
        GUIImage* element = new GUIImage(id, options, assetManager, name,
                                         fileName);
        element->setFullScreen(fullScreen);
        element->getTransformation()->setTranslate(tr.getTranslate());
        element->getTransformation()->setOrientation(tr.getOrientation());
        element->getTransformation()->setScale(tr.getScale());

        return element;
    }

    //unknown type case
    return nullptr;
}

GameObject::ImGuiResult GUIImage::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;

    //double # because I don't want to show it
    ImGui::InputText("Name##SelectedGUIImageNameField", GUINameBuffer, sizeof(GUINameBuffer));
    this->name = GUINameBuffer;



    //double # because I don't want to show it
    ImGui::InputText("File##SelectedGUIImageFileField", GUIFileNameBuffer, sizeof(GUIFileNameBuffer));
    if(ImGui::Button("change image")) {
        std::string enteredFileName = std::string(GUIFileNameBuffer);
        TextureAsset *newAsset = assetManager->loadAsset<TextureAsset>({enteredFileName});
        if (newAsset != nullptr) {
            assetManager->freeAsset({image->getName()});
            this->image = newAsset;
            this->setScale(newAsset->getHeight() / 2.0f, newAsset->getWidth() /
                                                         2.0f);// split in half, because the quad is -1 to 1, meaning it is 2 units long.
             this->imageFile = enteredFileName;
        } else {
            std::cerr << "New image file can't be read" << std::endl;
        }
    }

    bool fullScreenEditor = this->fullScreen;
    ImGui::Checkbox("full screen", &fullScreenEditor);
    if(this->fullScreen != fullScreenEditor) {
        this->setFullScreen(fullScreenEditor);
    }

    if(!this->fullScreen) {
        glm::vec2 translate = getTranslate();
        result.updated = ImGui::DragFloat("Position X", &(translate.x), 0, request.screenWidth) || result.updated;
        result.updated = ImGui::DragFloat("Position Y", &(translate.y), 0, request.screenHeight) || result.updated;

        glm::vec2 scale = getScale();
        result.updated = ImGui::DragFloat("Scale X", &(scale.x), 0.1, 10) || result.updated;
        result.updated = ImGui::DragFloat("Scale Y", &(scale.y), 0.1, 10) || result.updated;

        ImGui::NewLine();

        if (result.updated) {
            this->setTranslate(translate);
            this->setScale(scale);
            result.updated = true;
        }

        /* IMGUIZMO PART */

        static bool useSnap; //these are static because we want to keep the values
        static float snap[3] = {50.0f, 50.0f, 50.0f};
        ImGui::NewLine();
        ImGui::Checkbox("", &(useSnap));
        ImGui::SameLine();
        ImGui::InputFloat3("Snap", &(snap[0]));

        glm::mat4 objectMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translate, 0));
        ImGuizmo::BeginFrame();
        static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

        ImGuiIO &io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuizmo::SetOrthographic(true);
        ImGuizmo::Manipulate(glm::value_ptr(request.ortogonalCameraMatrix), glm::value_ptr(request.ortogonalMatrix),
                             ImGuizmo::TRANSLATE, mCurrentGizmoMode, glm::value_ptr(objectMatrix), NULL,
                             useSnap ? &(snap[0]) : NULL);
        ImGuizmo::SetOrthographic(false);
        //now we should have object matrix updated, update the object

        //just clip the values
        if (objectMatrix[3][0] < 0) {
            objectMatrix[3][0] = 0;
        }
        if (objectMatrix[3][1] < 0) {
            objectMatrix[3][1] = 0;
        }
        if (objectMatrix[3][0] > request.screenWidth) {
            objectMatrix[3][0] = request.screenWidth;
        }
        if (objectMatrix[3][1] > request.screenHeight) {
            objectMatrix[3][1] = request.screenHeight;
        }

        this->setTranslate(glm::vec2(objectMatrix[3][0], objectMatrix[3][1]));
    }

    if (ImGui::Button("Remove")) {
        result.remove = true;
    }

    return result;
}

void GUIImage::setFullScreen(bool fullScreen) {
    if(fullScreen) {
        this->fullScreen = true;
        this->setTranslate(glm::vec2(options->getScreenWidth() / 2.0f, options->getScreenHeight() / 2.0f));
        this->setScale(this->getHeight() * (options->getScreenHeight() / this->getHeight()) / 2, this->getWidth()* (options->getScreenWidth() / this->getWidth()) / 2);
    } else {
        this->fullScreen = false;
        this->setScale(image->getHeight() /2.0f,image->getWidth() /2.0f);// split in half, because the quad is -1 to 1, meaning it is 2 units long.
    }
}
