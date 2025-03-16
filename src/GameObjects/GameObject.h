//
// Created by engin on 8.03.2018.
//

#ifndef LIMONENGINE_GAMEOBJECT_H
#define LIMONENGINE_GAMEOBJECT_H

#include <string>
#include <list>
#include <Utils/HardCodedTags.h>

#include "limonAPI/LimonAPI.h"
#include "Editor/ImGuiRequest.h"
#include "Editor/ImGuiResult.h"
#include "Utils/HashUtil.h"

/**
 * This class is used to provide a polymorphic way of determining type and name of the object.
 */
class GameObject {
public:
    enum class ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX, TRIGGER, GUI_TEXT, GUI_IMAGE, GUI_BUTTON, GUI_ANIMATION, SOUND, MODEL_GROUP, PARTICLE_EMITTER, GPU_PARTICLE_EMITTER };

    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual ImGuiResult addImGuiEditorElements(const ImGuiRequest &request [[gnu::unused]]) {ImGuiResult imGuiResult; return imGuiResult;};

    virtual void interact(LimonAPI *limonAPI [[gnu::unused]], std::vector<LimonTypes::GenericParameter> &interactionData [[gnu::unused]]) {};

    virtual uint32_t getWorldObjectID() const = 0;
    virtual ~GameObject() = default;

    virtual void addTag(const std::string& text) {
        HashUtil::HashedString tag(text);
        bool found = false;
        for (const HashUtil::HashedString& hashedString:tags) {
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

    /**
     * This method is here only for Editor. Don't use in game code, or refactor to remove rehashing of tags
     * @return list of tags, filtered by HardCodedTags list.
     */
    std::list<HashUtil::HashedString> getTagsCustomOnly() const {
        std::list<HashUtil::HashedString> filteredTags;
        for (const auto& currentTag: tags) {
            if (std::find(HardCodedTags::ALL_TAGS.begin(), HardCodedTags::ALL_TAGS.end(), currentTag.text) == HardCodedTags::ALL_TAGS.end()) {
                filteredTags.emplace_back(currentTag.text);
            }
        }
        return filteredTags;
    }

    virtual void removeTag(const std::string& text) {
        HashUtil::HashedString tag(text);
        bool found = false;
        for (std::list<HashUtil::HashedString>::const_iterator it = tags.begin(); it != tags.end(); ++it) {
            if(it->hash == tag.hash) {
                //found case
                found = true;
                tags.erase(it);
                break;
            }
        }
        if(!found) {
            std::cerr << "Tag removal fail because tag is not found " << text  << std::endl;
        }
    }
private:
    std::list<HashUtil::HashedString> tags;

};

#endif //LIMONENGINE_GAMEOBJECT_H
