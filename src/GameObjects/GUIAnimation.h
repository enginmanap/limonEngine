//
// Created by engin on 28.10.2018.
//

#ifndef LIMONENGINE_GUIANIMATION_H
#define LIMONENGINE_GUIANIMATION_H


#include "../GUI/GUIImageBase.h"
#include "GameObject.h"

class GUIAnimation : public GUIImageBase, public GameObject{

    uint32_t worldID;
    std::string name;
    long creationTime;
    uint32_t imagePerFrame;
    uint32_t duration;
    bool looped;
    std::vector<GUILayer*> parentLayers;
    //ATTENTION don't use imageFile variable use this one
    std::vector<std::string> imageFiles;
    //ATTENTION don't use image variable, use this one
    std::vector<TextureAsset *> images;

    //Editor variables
    char GUINameBuffer[128];
    char* GUIFileNameBuffer[256]; //activate per element

public:
    GUIAnimation(uint32_t worldID,  std::shared_ptr<AssetManager> assetManager, const std::string name,
                     const std::vector<std::string> &imageFiles, long creationTime, uint32_t frameSpeed,
                     bool isLooped);

    void setupForTime(long time);


    ~GUIAnimation();

    void addedToLayer(GUILayer* layer);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static GUIAnimation *
    deserialize(tinyxml2::XMLElement *GUIRenderableNode,  std::shared_ptr<AssetManager> assetManager, Options *options); //will turn into factory class at some point

    /******************** Game object methods ************************************/
    ObjectTypes getTypeID() const override;

    std::string getName() const override;

    uint32_t getWorldObjectID() const override {
        return worldID;
    }

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request);

/******************** Game object methods ************************************/
};


#endif //LIMONENGINE_GUIANIMATION_H
