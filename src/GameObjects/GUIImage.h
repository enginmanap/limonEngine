//
// Created by engin on 26.07.2018.
//

#ifndef LIMONENGINE_GUIIMAGE_H
#define LIMONENGINE_GUIIMAGE_H


#include "../GUI/GUIImageBase.h"
#include "GameObject.h"

class GUIImage : public GUIImageBase, public GameObject {
    uint32_t worldID;
    std::string name;
    std::vector<GUILayer*> parentLayers;

    //Editor variables
    char GUINameBuffer[128];
    char GUIFileNameBuffer[256];

public:
    GUIImage(uint32_t worldID, GLHelper *glHelper, AssetManager *assetManager, const std::string name,
                 const std::string &imageFile)
            : GUIImageBase(
            glHelper, assetManager, imageFile), worldID(worldID), name(name) {
        strncpy(GUINameBuffer, this->name.c_str(), sizeof(GUINameBuffer));
        strncpy(GUIFileNameBuffer, this->imageFile.c_str(), sizeof(GUIFileNameBuffer));

    }

    ~GUIImage();

    void addedToLayer(GUILayer* layer);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static GUIImage *deserialize(tinyxml2::XMLElement *GUIRenderableNode, AssetManager *assetManager, Options *options); //will turn into factory class at some point


    /******************** Game object methods ************************************/
    ObjectTypes getTypeID() const override;

    std::string getName() const override;

    uint32_t getWorldObjectID() override;

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request);

/******************** Game object methods ************************************/
};


#endif //LIMONENGINE_GUIIMAGE_H
