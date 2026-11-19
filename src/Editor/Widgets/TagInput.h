//
// Created by engin on 14.11.2026.
//

#ifndef LIMONENGINE_TAGINPUT_H
#define LIMONENGINE_TAGINPUT_H

#include <cstdint>
#include <set>
#include <string>
#include <vector>

struct TagChip {
    enum class Kind { INHERITED, ADDED, REMOVED };
    std::string text;
    Kind kind;
    std::string tooltip;
    //only the model panel sets these. If some of the model's meshes have the tag but not all, the chip shows "n/m" and a + that adds it to the rest
    uint32_t meshCount = 0;//meshes that have this tag
    uint32_t totalMeshCount = 0;//meshes the model has

    bool isPartial() const {
        return meshCount > 0 && meshCount < totalMeshCount;
    }
};

struct TagInputResult {
    enum class Action { NONE, ADD, REMOVE };
    Action action = Action::NONE;
    std::string tag;
};

class TagInput {
public:
    //the typed text is kept per ImGui ID, so push something unique to the edited thing before calling.
    //knownTags can be null, then there is nothing to suggest. Added tags are put in it
    static TagInputResult draw(const char *label, const std::vector<TagChip> &chips, std::set<std::string> *knownTags);

private:
    static float chipWidth(const TagChip &chip);
    static bool drawChip(const TagChip &chip, bool &closeClicked, bool &addToAllClicked);
    static std::string countText(const TagChip &chip);
    static bool isSuggestable(const std::string &knownTag, const std::string &typed, const std::vector<TagChip> &chips);
};

#endif //LIMONENGINE_TAGINPUT_H
