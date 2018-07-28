//
// Created by engin on 28.07.2018.
//

#ifndef LIMONENGINE_GUIBUTTON_H
#define LIMONENGINE_GUIBUTTON_H


#include "../GUI/GUIImageBase.h"
#include "GameObject.h"

class GUIButton : public GUIImageBase, public GameObject{
    uint32_t worldID;
    std::string name;
    std::vector<GUILayer*> parentLayers;
    //ATTENTION don't use imageFile variable use this one
    std::vector<std::string> imageFiles;
    //ATTENTION don't use image variable, use this one
    std::vector<TextureAsset *> images = {0};

    const char editorFileNameFields[4][45] = {"Normal file##SelectedGUIButtonFileField", "On hover file##SelectedGUIButtonFileField", "On click file##SelectedGUIButtonFileField", "Disabled File##SelectedGUIButtonFileField"};
    const char editorApplyFields[4][40] = {"Update normal##UpdateGUIButtonField", "Update on hover##UpdateGUIButtonField", "Update on click##UpdateGUIButtonField", "Update disabled##UpdateGUIButtonField"};



    //Editor variables
    char GUINameBuffer[128];
    char GUIFileNameBuffer[4][256] = {0};

public:
    GUIButton(uint32_t worldID, AssetManager *assetManager, const std::string name,
              const std::vector<std::string> &imageFiles);

    ~GUIButton();

    void addedToLayer(GUILayer* layer);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static GUIButton *deserialize(tinyxml2::XMLElement *GUIRenderableNode, AssetManager *assetManager, Options *options); //will turn into factory class at some point


    /******************** Game object methods ************************************/
    ObjectTypes getTypeID() const override;

    std::string getName() const override;

    uint32_t getWorldObjectID() override;

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request);

/******************** Game object methods ************************************/

};


#endif //LIMONENGINE_GUIBUTTON_H
