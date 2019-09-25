//
// Created by engin on 15.06.2018.
//

#ifndef LIMONENGINE_GUITEXT_H
#define LIMONENGINE_GUITEXT_H


#include "../GUI/GUITextBase.h"

class Options;

class GUIText : public GUITextBase, public GameObject {
    uint32_t worldID;
    std::string name;
    std::vector<GUILayer*> parentLayers;

public:
    GUIText(GraphicsInterface *glHelper, uint32_t id, const std::string &name, Face *font, const std::string &text,
            const glm::vec3 &color);

    void addedToLayer(GUILayer* layer);

    ~GUIText();

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static GUIText *deserialize(tinyxml2::XMLElement *GUIRenderableNode, GraphicsInterface *glHelper, FontManager *fontManager, Options *options); //will turn into factory class at some point


    /******************** Game object methods ************************************/
    ObjectTypes getTypeID() const override;

    std::string getName() const override;

    uint32_t getWorldObjectID() const override {
        return worldID;
    }

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request);

/******************** Game object methods ************************************/

};


#endif //LIMONENGINE_GUITEXT_H
