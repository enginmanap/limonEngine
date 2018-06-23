//
// Created by engin on 15.06.2018.
//

#include "GUIText.h"
#include "../GUI/GUILayer.h"
#include "../../libs/ImGui/imgui.h"
#include "../../libs/ImGuizmo/ImGuizmo.h"

GUIText::GUIText(GLHelper *glHelper, uint32_t id, const std::string &name, Face *font, const std::string &text,
                 const glm::vec3 &color)
        : GUITextBase(glHelper, font, text, color), worldID(id), name(name) {}

GameObject::ObjectTypes GUIText::getTypeID() const {
    return GUI_TEXT;
}

std::string GUIText::getName() const {
    return this->name;
}

uint32_t GUIText::getWorldObjectID() {
    return worldID;
}

void GUIText::addedToLayer(GUILayer *layer) {
    parentLayers.push_back(layer);
}


GUIText::~GUIText() {
    for (size_t i = 0; i < parentLayers.size(); ++i) {
        parentLayers[i]->removeGuiElement(this->getWorldObjectID());
    }
}

bool GUIText::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options) {
    tinyxml2::XMLElement *guiTextNode = document.NewElement("GUIElement");
    parentNode->InsertEndChild(guiTextNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("Type");
    currentElement->SetText("GUIText");
    guiTextNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(getWorldObjectID());
    guiTextNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Name");
    currentElement->SetText(this->name.c_str());
    guiTextNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Text");
    currentElement->SetText(this->text.c_str());
    guiTextNode->InsertEndChild(currentElement);

    tinyxml2::XMLElement *fontNode = document.NewElement("Font");
    guiTextNode->InsertEndChild(fontNode);

    currentElement = document.NewElement("Path");
    currentElement->SetText(this->face->getPath().c_str());
    fontNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Size");
    currentElement->SetText(std::to_string(this->face->getSize()).c_str());
    fontNode->InsertEndChild(currentElement);

    tinyxml2::XMLElement * parent = document.NewElement("Color");
    currentElement = document.NewElement("R");
    currentElement->SetText(color.r * 256);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("G");
    currentElement->SetText(color.g * 256);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("B");
    currentElement->SetText(color.b * 256);
    parent->InsertEndChild(currentElement);
    guiTextNode->InsertEndChild(parent);

    Transformation temp(transformation);

    temp.setTranslate(glm::vec3(
            temp.getTranslate().x / options->getScreenWidth(),
            temp.getTranslate().y / options->getScreenHeight(),
            temp.getTranslate().z
                      ));

    temp.serialize(document, guiTextNode);
    return true;
}


GUIText *GUIText::deserialize(tinyxml2::XMLElement *GUIRenderableNode, GLHelper *glHelper, FontManager *fontManager, Options *options) {

    tinyxml2::XMLElement* GUIRenderableAttribute;

    GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Type");
    if (GUIRenderableAttribute == nullptr) {
        std::cerr << "GUI renderable must have a type. Skipping" << std::endl;
        return nullptr;
    }
    std::string type = GUIRenderableAttribute->GetText();
    if(type == "GUIText") {
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

        std::string text;

        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Text");
        if (GUIRenderableAttribute == nullptr) {
            std::cout << "GUI Text without text found, assuming empty" << std::endl;
        } else {
            text = GUIRenderableAttribute->GetText();
        }

        //now get the font
        tinyxml2::XMLElement* FontAttribute = GUIRenderableNode->FirstChildElement("Font");


        GUIRenderableAttribute = FontAttribute->FirstChildElement("Path");
        if (GUIRenderableAttribute == nullptr) {
            std::cout << "GUI Text Font path can't be read. Skipping" << std::endl;
            return nullptr;
        }
        std::string path = GUIRenderableAttribute->GetText();

        uint32_t size;
        GUIRenderableAttribute = FontAttribute->FirstChildElement("Size");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI Text font size can't be read. Assumin 32" << std::endl;
            size = 32;
        } else {
            size = std::stoi(GUIRenderableAttribute->GetText());
        }

        //now read the color information
        glm::vec3 color;

        tinyxml2::XMLElement* colorNode = GUIRenderableNode->FirstChildElement("Color");
        if (colorNode == nullptr) {
            color.x = color.y = color.z = 0.0f;
        } else {
            GUIRenderableAttribute = colorNode->FirstChildElement("R");
            if (GUIRenderableAttribute != nullptr) {
                color.x = std::stof(GUIRenderableAttribute->GetText());
            } else {
                color.x = 0.0f;
            }
            GUIRenderableAttribute = colorNode->FirstChildElement("G");
            if (GUIRenderableAttribute != nullptr) {
                color.y = std::stof(GUIRenderableAttribute->GetText());
            } else {
                color.y = 0.0f;
            }
            GUIRenderableAttribute = colorNode->FirstChildElement("B");
            if (GUIRenderableAttribute != nullptr) {
                color.z = std::stof(GUIRenderableAttribute->GetText());
            } else {
                color.z = 0.0f;
            }
        }

        GUIRenderableAttribute =  GUIRenderableNode->FirstChildElement("Transformation");
        if(GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI Text does not have transformation. Skipping" << std::endl;
            return nullptr;
        }
        Transformation tr;
        tr.deserialize(GUIRenderableAttribute);

        tr.setTranslate(glm::vec3(
                tr.getTranslate().x * options->getScreenWidth(),
                tr.getTranslate().y * options->getScreenHeight(),
                tr.getTranslate().z
        ));
        //now we have everything, create the GUI Text
        GUIText* element = new GUIText(glHelper, id, name, fontManager->getFont(path, size), text, color);
        element->getTransformation()->setTranslate(tr.getTranslate());
        element->getTransformation()->setOrientation(tr.getOrientation());
        return element;
    }

    //unknown type case
    return nullptr;
}

GameObject::ImGuiResult GUIText::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;


    char GUINameBuffer[128];
    strncpy(GUINameBuffer, this->name.c_str(), sizeof(GUINameBuffer));
    //double # because I don't want to show it
    ImGui::InputText("Name##SelectedGUITextNameField", GUINameBuffer, sizeof(GUINameBuffer));

    this->name = GUINameBuffer;
    
    char GUITextBuffer[128];
    strncpy(GUITextBuffer, this->text.c_str(), sizeof(GUITextBuffer));

    //double # because I don't want to show it
    ImGui::InputText("Text##SelectedGUITextTextField", GUITextBuffer, sizeof(GUITextBuffer));

    this->updateText(GUITextBuffer);

    glm::vec2 translate = getTranslate();
    result.updated = ImGui::DragFloat("Position X", &(translate.x), 0,request.screenWidth)   || result.updated;
    result.updated = ImGui::DragFloat("Position Y", &(translate.y), 0,request.screenHeight)   || result.updated;

    ImGui::NewLine();
    result.updated = ImGui::SliderFloat("Color R", &(this->color.r), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::SliderFloat("Color G", &(this->color.g), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::SliderFloat("Color B", &(this->color.b), 0.0f, 1.0f)   || result.updated;
    ImGui::NewLine();

    if(result.updated) {
        this->setTranslate(translate);
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

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::SetOrthographic(true);
    ImGuizmo::Manipulate(glm::value_ptr(request.ortogonalCameraMatrix), glm::value_ptr(request.ortogonalMatrix), ImGuizmo::TRANSLATE, mCurrentGizmoMode, glm::value_ptr(objectMatrix), NULL, useSnap ? &(snap[0]) : NULL);
    ImGuizmo::SetOrthographic(false);
    //now we should have object matrix updated, update the object

    //just clip the values
    if(objectMatrix[3][0] <0 ) {
        objectMatrix[3][0] = 0;
    }
    if(objectMatrix[3][1] <0 ) {
        objectMatrix[3][1] = 0;
    }
    if(objectMatrix[3][0] > request.screenWidth ) {
        objectMatrix[3][0] = request.screenWidth;
    }
    if(objectMatrix[3][1] > request.screenHeight) {
        objectMatrix[3][1] = request.screenHeight;
    }

    this->setTranslate(glm::vec2(objectMatrix[3][0], objectMatrix[3][1]));

    return result;
}