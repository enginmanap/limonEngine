//
// Created by engin on 8.03.2018.
//

#ifndef LIMONENGINE_GAMEOBJECT_H
#define LIMONENGINE_GAMEOBJECT_H

#include <string>

#include "API/LimonAPI.h"
#include "Editor/ImGuiRequest.h"
#include "Editor/ImGuiResult.h"
#include "Utils/HashUtil.h"

/**
 * This class is used to provide a polymorphic way of determining type and name of the object.
 */
class GameObject {
public:


    enum ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX, TRIGGER, GUI_TEXT, GUI_IMAGE, GUI_BUTTON, GUI_ANIMATION, SOUND, MODEL_GROUP, PARTICLE_EMITTER, GPU_PARTICLE_EMITTER };

    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual ImGuiResult addImGuiEditorElements(const ImGuiRequest &request [[gnu::unused]]) {ImGuiResult imGuiResult; return imGuiResult;};

    virtual void interact(LimonAPI *limonAPI [[gnu::unused]], std::vector<LimonTypes::GenericParameter> &interactionData [[gnu::unused]]) {};

    virtual uint32_t getWorldObjectID() const = 0;
    virtual ~GameObject() = default;

    void addTag(const std::string& text) {
        HashUtil::HashedString tag(text);
        bool found = false;
        for (HashUtil::HashedString hashedString:tags) {
            if(hashedString.hash == tag.hash) {
                if(hashedString.text != tag.text) {
                    std::cerr << "Hash collision found between " << hashedString.text << " and " << tag.text << " exiting." << std::endl;
                    std::exit(-1);
                }
                //found case
                found = true;
                break;
            }
        }
        if(!found) {
            tags.emplace_back(tag);
        }
    }

    bool hasTag(uint64_t hash) {
        for (const HashUtil::HashedString& hashedString:tags) {
            if(hashedString.hash == hash) {
                return true;
            }
        }
        return false;
    }

    const std::vector<HashUtil::HashedString>& getTags() {
        return tags;
    }
private:
    std::vector<HashUtil::HashedString> tags;

};

#endif //LIMONENGINE_GAMEOBJECT_H
