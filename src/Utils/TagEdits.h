//
// Created by engin on 14.11.2026.
//

#ifndef LIMONENGINE_TAGEDITS_H
#define LIMONENGINE_TAGEDITS_H

#include <string>
#include <vector>
#include <cstdint>

#include "Utils/HashUtil.h"

struct TagEdit {
    std::string text;
    uint64_t hash;
    bool added;//false means removed, even when nothing below provides the tag
};

class TagEdits {
public:
    //one entry per tag, the last add or remove wins
    static void set(std::vector<TagEdit> &edits, const std::string &text, bool added) {
        uint64_t hash = HashUtil::hashString(text);
        for (TagEdit &edit : edits) {
            if (edit.hash == hash) {
                edit.added = added;
                return;
            }
        }
        edits.push_back(TagEdit{text, hash, added});
    }

    static bool erase(std::vector<TagEdit> &edits, uint64_t hash) {
        for (std::vector<TagEdit>::iterator editIt = edits.begin(); editIt != edits.end(); ++editIt) {
            if (editIt->hash == hash) {
                edits.erase(editIt);
                return true;
            }
        }
        return false;
    }

    static void addUnique(std::vector<TagEdit> &tags, const std::string &text, uint64_t hash) {
        for (const TagEdit &tag : tags) {
            if (tag.hash == hash) {
                return;
            }
        }
        tags.push_back(TagEdit{text, hash, true});
    }

    static void addUnique(std::vector<TagEdit> &tags, const std::string &text) {
        addUnique(tags, text, HashUtil::hashString(text));
    }

    static bool contains(const std::vector<TagEdit> &tags, uint64_t hash) {
        for (const TagEdit &tag : tags) {
            if (tag.hash == hash) {
                return true;
            }
        }
        return false;
    }

    //tags is a working list, every entry in it is an added one
    static void apply(const std::vector<TagEdit> &edits, std::vector<TagEdit> &tags) {
        for (const TagEdit &edit : edits) {
            if (edit.added) {
                addUnique(tags, edit.text, edit.hash);
            } else {
                erase(tags, edit.hash);
            }
        }
    }
};

#endif //LIMONENGINE_TAGEDITS_H
