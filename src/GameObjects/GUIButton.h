//
// Created by engin on 28.07.2018.
//

#ifndef LIMONENGINE_GUIBUTTON_H
#define LIMONENGINE_GUIBUTTON_H


#include "../GUI/GUIImageBase.h"
#include "GameObject.h"
#include "API/LimonAPI.h"
#include "API/TriggerInterface.h"

class LimonAPI;

class GUIButton : public GUIImageBase, public GameObject{

    bool onHover = false;
    bool onClick = false;
    bool enabled = false;
    uint32_t worldID;
    std::string name;
    std::vector<GUILayer*> parentLayers;
    //ATTENTION don't use imageFile variable use this one
    std::vector<std::string> imageFiles;
    //ATTENTION don't use image variable, use this one
    std::vector<TextureAsset *> images;

    LimonAPI* limonAPI;
    TriggerInterface* onClickTriggerCode = nullptr;
    std::vector<LimonAPI::ParameterRequest> onClickParameters;


    const char editorFileNameFields[4][45] = {"Normal file##SelectedGUIButtonFileField", "On hover file##SelectedGUIButtonFileField", "On click file##SelectedGUIButtonFileField", "Disabled File##SelectedGUIButtonFileField"};
    const char editorApplyFields[4][40] = {"Update normal##UpdateGUIButtonField", "Update on hover##UpdateGUIButtonField", "Update on click##UpdateGUIButtonField", "Update disabled##UpdateGUIButtonField"};



    //Editor variables
    char GUINameBuffer[128];
    char GUIFileNameBuffer[4][256] = {{0}};

    void setImageFromFlags() {

        if(!this->enabled ) {
            if(this->images.size() > 3) {
                this->image = this->images[3];
            }
        } else if(this->onClick) {
            if(this->images.size() > 2) {
                this->image = this->images[2];
            }
        } else if(this->onHover) {
            if(this->images.size() > 1) {
                this->image = this->images[1];
            }
        } else {
            this->image = this->images[0];
        }
    }

public:
    GUIButton(uint32_t worldID, AssetManager *assetManager, LimonAPI *limonAPI, const std::string name,
                  const std::vector<std::string> &imageFiles);

    ~GUIButton();

    void addedToLayer(GUILayer* layer);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static GUIButton *deserialize(tinyxml2::XMLElement *GUIRenderableNode, AssetManager *assetManager, Options *options,
                                      LimonAPI *limonAPI); //will turn into factory class at some point

    void setOnHover(bool hover) {
        this->onHover = hover;
        this->setImageFromFlags();
    }

    void setOnClick(bool click) {
        this->onClick = click;
        this->setImageFromFlags();
        if(this->onClickTriggerCode != nullptr && enabled == true && click == true) {
            this->onClickTriggerCode->run(onClickParameters);
        }
    }

    void setEnabled(bool enabled) {
        this->enabled = enabled;
        this->setImageFromFlags();
    }

    /******************** Game object methods ************************************/
    ObjectTypes getTypeID() const override;

    std::string getName() const override;

    uint32_t getWorldObjectID() const override {
        return worldID;
    }

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request);

/******************** Game object methods ************************************/

};


#endif //LIMONENGINE_GUIBUTTON_H
