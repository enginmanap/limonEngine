//
// Created by engin on 8.03.2018.
//

#ifndef LIMONENGINE_GAMEOBJECT_H
#define LIMONENGINE_GAMEOBJECT_H

#include <string>
#include <list>

#include "limonAPI/LimonAPI.h"
#include "Editor/ImGuiRequest.h"
#include "Editor/ImGuiResult.h"
#include "Editor/EditorRenderable.h"
#include "Utils/HashUtil.h"

/**
 * This class is used to provide a polymorphic way of determining type and name of the object.
 */
class GameObject : public EditorRenderable {
public:
    enum class ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX, TRIGGER, GUI_TEXT, GUI_IMAGE, GUI_BUTTON, GUI_ANIMATION, SOUND, MODEL_GROUP, PARTICLE_EMITTER, GPU_PARTICLE_EMITTER, CAMERA_RIG };

    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual ImGuiResult addImGuiEditorElements(const ImGuiRequest &request [[gnu::unused]]) override {ImGuiResult imGuiResult; return imGuiResult;};

    virtual void interact(LimonAPI *limonAPI [[gnu::unused]], std::vector<LimonTypes::GenericParameter> &interactionData [[gnu::unused]]) {};

    virtual uint32_t getWorldObjectID() const = 0;
    virtual ~GameObject() = default;

    virtual bool addTag(const std::string& text) {
        HashUtil::HashedString tag(text);
        for (const HashUtil::HashedString& hashedString:tags) {
            if(hashedString.hash == tag.hash) {
                if(hashedString.text != tag.text) {
                    std::cerr << "Hash collision found between " << hashedString.text << " and " << tag.text << std::endl;
                    //On debug builds, we kill as soon as a collision is found, so it would be easy to catch.
#ifndef NDEBUG
                    std::exit(-1);
#endif
                    return false;
                }
                return true;//already have that tag, no need to add again.
            }
        }
        tags.emplace_back(tag);
        return true;
    }

    bool hasTag(uint64_t hash) const {
        for (const HashUtil::HashedString& hashedString:tags) {
            if(hashedString.hash == hash) {
                return true;
            }
        }
        return false;
    }

    /**
     *
     * @return all tags, including hardcoded ones
     */
    const std::list<HashUtil::HashedString>& getTags() const {
        return tags;
    }

    virtual bool removeTag(const std::string& text) {
        HashUtil::HashedString tag(text);
        for (std::list<HashUtil::HashedString>::const_iterator it = tags.begin(); it != tags.end(); ++it) {
            if(it->hash == tag.hash) {
                tags.erase(it);
                return true;
            }
        }
        std::cerr << "Tag removal fail because tag is not found " << text  << std::endl;
        return false;
    }
private:
    std::list<HashUtil::HashedString> tags;

};

#endif //LIMONENGINE_GAMEOBJECT_H
