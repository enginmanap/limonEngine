//
// Created by engin on 26.07.2018.
//

#ifndef LIMONENGINE_GUIIMAGE_H
#define LIMONENGINE_GUIIMAGE_H


#include "../GUI/GUIImageBase.h"
#include "GameObject.h"

class Options;
class GUIImage : public GUIImageBase, public GameObject {
    uint32_t worldID;
    std::string name;
    Options* options;
    std::vector<GUILayer*> parentLayers;

    bool fullScreen = false;
    //Editor variables
    char GUINameBuffer[128];
    char GUIFileNameBuffer[256];

public:
    GUIImage(uint32_t worldID, Options *options, AssetManager *assetManager, const std::string name,
             const std::string &imageFile);

    ~GUIImage();

    void addedToLayer(GUILayer* layer);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static GUIImage *deserialize(tinyxml2::XMLElement *GUIRenderableNode, AssetManager *assetManager, Options *options); //will turn into factory class at some point


    /******************** Game object methods ************************************/
    ObjectTypes getTypeID() const override;

    std::string getName() const override;

    uint32_t getWorldObjectID() const override {
        return worldID;
    }

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;

/******************** Game object methods ************************************/
    void setFullScreen(bool fullScreen);
};


#endif //LIMONENGINE_GUIIMAGE_H
