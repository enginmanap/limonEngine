//
// Created by engin on 8.06.2018.
//

#ifndef LIMONENGINE_ANIMATIONSEQUENCE_H
#define LIMONENGINE_ANIMATIONSEQUENCE_H


#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../libs/ImGuizmo/ImSequencer.h"
#include "Transformation.h"
#include "Assets/Animations/AnimationCustom.h"

class Renderable;
class AnimationNode;

class AnimationSequenceInterface : public ImSequencer::SequenceInterface {
    static const uint32_t NO_SECTION = UINT32_MAX;
    static const int NEW_SECTION_LENGTH = 60;

    int startTime = 0;
    Transformation originalTransformation;
    struct AnimationSequenceItem {
        uint32_t sectionID;
        int mFrameEnd;
        glm::mat4 relativeTransform;//from where the previous section ends, first one from the original
        char name[32];
    };
    std::vector<AnimationSequenceItem> sections;
    Renderable* animatingObject;
    uint32_t animatingObjectID;
    uint32_t nextSectionID = 0;

    /********************IMGUI variables *************************/
    char animationNameBuffer[32] {0};
    int selectedEntry = 0;//to show this, we must have created 1 element. that one is selected
    uint32_t selectedSectionID = NO_SECTION;//ImSequencer only gives us an index, deleting shifts what it points to
    bool sectionDeleted = false;
    int firstFrame = 0;
    bool expanded = true;
    /********************IMGUI variables *************************/

    AnimationSequenceItem createSection(int frameEnd);
    glm::mat4 getOriginalLocal() const;
    glm::mat4 getSectionEndTransform(int sectionIndex) const;//relative to original, -1 is the original itself
    void moveObjectToSelectedSection();
    void restoreOriginalTransformation();
    bool objectMoved = false;
    std::vector<int> getBoundaries() const;//start time, then every section's end
    void pushLaterSections(const std::vector<int> &boundariesBefore);
    void pushMatrixToAnimationTime(std::shared_ptr<AnimationNode> animationNode, float time, const glm::mat4 &transform) const;


public:
    int mFrameCount = 120;

    int GetFrameCount() const { return mFrameCount; }
    int GetItemCount() const override { return (int)sections.size(); }

    int GetItemTypeCount() const override { return 1; }
    const char *GetItemTypeName(int typeIndex [[gnu::unused]]) const override { return "Section"; }
    const char *GetItemLabel(int index) const override {
        static char tmps[512];
        snprintf(tmps, sizeof(tmps), "[%02d] %s", index, sections[index].name);
        return tmps;
    }

    void Get(int index, int** start, int** end, int *type, unsigned int *color) override {
        AnimationSequenceItem &item = sections[index];
        if (color) {
            *color = 0xFFAA8080; // same color for everyone, return color based on type
        }
        if (start) {
            if(index == 0) {
                *start = &startTime;
            } else {
                *start = &sections[index-1].mFrameEnd;
            }
        }
        if (end) {
            *end = &item.mFrameEnd;
        }
        if (type) {
            *type = 0;//we don't have multiple types
        }
    }
    void Add(int type [[gnu::unused]]) override;
    void Del(int index) override;
    void Duplicate(int index) override;//the + on a section row, we insert an empty section after it instead of copying

    Renderable *getAnimatingObject() const {
        return animatingObject;
    }

    uint32_t getAnimatingObjectID() const {
        return animatingObjectID;
    }

    void saveObjectToSelectedSection();

    AnimationSequenceInterface(Renderable* animatingObject, uint32_t animatingObjectID);
    ~AnimationSequenceInterface() override;
    AnimationCustom buildAnimationFromCurrentItems() const;

    bool addAnimationSequencerToEditor(bool &finished, bool &cancelled);//true if it moved the object

    int GetFrameMin() const override {return 0;};
    int GetFrameMax() const override {return mFrameCount;};
};


#endif //LIMONENGINE_ANIMATIONSEQUENCE_H
