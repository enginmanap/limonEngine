//
// Created by engin on 15.06.2018.
//

#include "GUIText.h"
#include "../GUI/GUILayer.h"
#include "../../libs/ImGui/imgui.h"
#include "../../libs/ImGuizmo/ImGuizmo.h"

GUIText::GUIText(OpenGLGraphics *glHelper, uint32_t id, const std::string &name, Face *font, const std::string &text,
                 const glm::vec3 &color)
        : GUITextBase(glHelper, font, text, color), worldID(id), name(name) {}

GameObject::ObjectTypes GUIText::getTypeID() const {
    return GUI_TEXT;
}

std::string GUIText::getName() const {
    return this->name;
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
    float tempSize = this->face->getSize();
    tempSize = tempSize / options->getScreenWidth();
    currentElement->SetText(std::to_string(tempSize).c_str());
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


GUIText *GUIText::deserialize(tinyxml2::XMLElement *GUIRenderableNode, OpenGLGraphics *glHelper, FontManager *fontManager, Options *options) {

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
            std::cerr << "GUI Text font size can't be read. Assuming 32" << std::endl;
            size = 32;
        } else {
            float tempSize = std::stof(GUIRenderableAttribute->GetText());
            tempSize = tempSize * options->getScreenWidth();
            size = static_cast<int>(tempSize);
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
    strncpy(GUINameBuffer, this->name.c_str(), sizeof(GUINameBuffer) - 1);
    //double # because I don't want to show it
    ImGui::InputText("Name##SelectedGUITextNameField", GUINameBuffer, sizeof(GUINameBuffer));

    this->name = GUINameBuffer;
    
    char GUITextBuffer[128];
    strncpy(GUITextBuffer, this->text.c_str(), sizeof(GUITextBuffer) - 1);

    //double # because I don't want to show it
    ImGui::InputText("Text##SelectedGUITextTextField", GUITextBuffer, sizeof(GUITextBuffer));

    this->updateText(GUITextBuffer);

    result.updated = this->transformation.addImGuiEditorElements(request.ortogonalCameraMatrix, request.ortogonalMatrix, true);


    ImGui::NewLine();
    result.updated = ImGui::SliderFloat("Color R", &(this->color.r), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::SliderFloat("Color G", &(this->color.g), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::SliderFloat("Color B", &(this->color.b), 0.0f, 1.0f)   || result.updated;
    ImGui::NewLine();
    float alphaTemp = this->transformation.getTranslateSingle().z;//since translate z is unused, we use it to store Alpha
    if(ImGui::SliderFloat("Alpha", &alphaTemp, 0.0f, 1.0f)) {
        this->transformation.setTranslate(glm::vec3(this->transformation.getTranslateSingle().x, this->transformation.getTranslateSingle().y, alphaTemp));
        result.updated = true;
    }

    ImGui::NewLine();
    if (ImGui::Button("Remove")) {
        result.remove = true;
    }

    return result;
}