//
// Created by engin on 14.11.2026.
//

#ifndef LIMONENGINE_TAGINPUT_H
#define LIMONENGINE_TAGINPUT_H

#include <set>
#include <string>
#include <vector>

struct TagChip {
    enum class Kind { INHERITED, ADDED, REMOVED };
    std::string text;
    Kind kind;
    std::string tooltip;
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
    static bool drawChip(const TagChip &chip, bool &closeClicked);
    static bool isSuggestable(const std::string &knownTag, const std::string &typed, const std::vector<TagChip> &chips);
};

#endif //LIMONENGINE_TAGINPUT_H
