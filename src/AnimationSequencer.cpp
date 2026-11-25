//
// Created by engin on 8.06.2018.
//

#include "AnimationSequencer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <algorithm>
#include <cmath>
#include "Renderable.h"
#include "../libs/ImGui/imgui.h"

static glm::mat4 composeLocalMatrix(const glm::vec3 &translate, const glm::quat &orientation, const glm::vec3 &scale) {
    return glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation) * glm::scale(glm::mat4(1.0f), scale);
}

AnimationSequenceInterface::AnimationSequenceInterface(Renderable* animatingObject, uint32_t animatingObjectID) :
        originalTransformation(*animatingObject->getTransformation()), animatingObject(animatingObject), animatingObjectID(animatingObjectID) {
    sections.push_back(createSection(30));//put one element to sections by default
    selectedSectionID = sections[0].sectionID;
}

AnimationSequenceInterface::AnimationSequenceInterface(Renderable* animatingObject, uint32_t animatingObjectID, const AnimationCustom &animationToEdit, uint32_t animationIndex) :
        originalTransformation(*animatingObject->getTransformation()), animatingObject(animatingObject), animatingObjectID(animatingObjectID),
        editedAnimationIndex(animationIndex), editedAnimationName(animationToEdit.getName()) {
    rebuildSectionsFromAnimation(animationToEdit);
    snprintf(animationNameBuffer, sizeof(animationNameBuffer), "%s", animationToEdit.getName().c_str());
    //object stands at the start, selecting a section would need a move
    selectedEntry = -1;
    selectedSectionID = NO_SECTION;
    mFrameCount = std::max(mFrameCount, sections.back().mFrameEnd);
}

//every key becomes a section end. The first key is where the object stands, so animations not starting from identity are re-based to it
void AnimationSequenceInterface::rebuildSectionsFromAnimation(const AnimationCustom &animation) {
    const AnimationNode &node = *animation.animationNode;
    std::vector<float> keyTimes;
    keyTimes.insert(keyTimes.end(), node.translateTimes.begin(), node.translateTimes.end());
    keyTimes.insert(keyTimes.end(), node.scaleTimes.begin(), node.scaleTimes.end());
    keyTimes.insert(keyTimes.end(), node.rotationTimes.begin(), node.rotationTimes.end());
    std::sort(keyTimes.begin(), keyTimes.end());
    keyTimes.erase(std::unique(keyTimes.begin(), keyTimes.end()), keyTimes.end());

    sections.clear();
    if (keyTimes.empty()) {
        sections.push_back(createSection(30));
        return;
    }
    float ticksPerSecond = animation.getTicksPerSecond() != 0 ? animation.getTicksPerSecond() : TICK_PER_SECOND;
    float frameScale = TICK_PER_SECOND / ticksPerSecond;//sections are in our ticks, the file may use another rate

    glm::mat4 previousPose = composeLocalMatrix(node.getPositionVector(keyTimes[0]), node.getRotationQuat(keyTimes[0]), node.getScalingVector(keyTimes[0]));
    int previousEnd = 0;
    int firstKeyFrame = (int)std::lround(keyTimes[0] * frameScale);
    if (firstKeyFrame > 0) {
        sections.push_back(createSection(firstKeyFrame));//the wait before the first key
        previousEnd = firstKeyFrame;
    }
    for (size_t i = 1; i < keyTimes.size(); i++) {
        glm::mat4 pose = composeLocalMatrix(node.getPositionVector(keyTimes[i]), node.getRotationQuat(keyTimes[i]), node.getScalingVector(keyTimes[i]));
        AnimationSequenceItem item = createSection(std::max((int)std::lround(keyTimes[i] * frameScale), previousEnd));
        item.relativeTransform = glm::inverse(previousPose) * pose;
        sections.push_back(item);
        previousPose = pose;
        previousEnd = item.mFrameEnd;
    }
    int durationFrame = (int)std::lround(animation.getDuration() * frameScale);
    if (durationFrame > previousEnd) {
        sections.push_back(createSection(durationFrame));//duration may run past the last key
    }
    if (sections.empty()) {
        sections.push_back(createSection(30));
    }
    //in order, so zero length sections sharing an end keep their own names. A section with no saved name, like the wait before the first key, keeps the default
    const std::vector<AnimationCustom::SectionName> &savedNames = animation.getSectionNames();
    size_t savedNameIndex = 0;
    for (AnimationSequenceItem &item : sections) {
        //zero length sections collapse into one key, their extra names have nothing to land on
        while (savedNameIndex < savedNames.size() && (int)std::lround(savedNames[savedNameIndex].frameEnd * frameScale) < item.mFrameEnd) {
            savedNameIndex++;
        }
        if (savedNameIndex < savedNames.size() && (int)std::lround(savedNames[savedNameIndex].frameEnd * frameScale) == item.mFrameEnd) {
            snprintf(item.name, sizeof(item.name), "%s", savedNames[savedNameIndex].name.c_str());
            savedNameIndex++;
        }
    }
}

AnimationSequenceInterface::~AnimationSequenceInterface(){
    if(animatingObject != nullptr) { //if cancelled or finalized, this is already done, and animating object is null
        restoreOriginalTransformation();
    }
}

AnimationSequenceInterface::AnimationSequenceItem AnimationSequenceInterface::createSection(int frameEnd) {
    AnimationSequenceItem item{nextSectionID++, frameEnd, glm::mat4(1.0f), {0}};
    snprintf(item.name, sizeof(item.name), "Section");
    return item;
}

//Single values, the original may sit on a parent and world values would apply it twice
glm::mat4 AnimationSequenceInterface::getOriginalLocal() const {
    return composeLocalMatrix(originalTransformation.getTranslateSingle(), originalTransformation.getOrientationSingle(), originalTransformation.getScaleSingle());
}

glm::mat4 AnimationSequenceInterface::getSectionEndTransform(int sectionIndex) const {
    glm::mat4 endTransform(1.0f);
    for (int i = 0; i <= sectionIndex; i++) {
        endTransform = endTransform * sections[i].relativeTransform;
    }
    return endTransform;
}

void AnimationSequenceInterface::saveObjectToSelectedSection() {
    if (selectedEntry < 0 || selectedEntry >= (int)sections.size()) {
        return;
    }
    const Transformation* current = animatingObject->getTransformation();
    glm::mat4 currentLocal = composeLocalMatrix(current->getTranslateSingle(), current->getOrientationSingle(), current->getScaleSingle());
    glm::mat4 sectionStart = getOriginalLocal() * getSectionEndTransform(selectedEntry - 1);
    sections[selectedEntry].relativeTransform = glm::inverse(sectionStart) * currentLocal;
}

void AnimationSequenceInterface::moveObjectToSelectedSection() {
    if (selectedEntry < 0 || selectedEntry >= (int)sections.size()) {
        restoreOriginalTransformation();
        return;
    }
    glm::mat4 objectLocal = getOriginalLocal() * getSectionEndTransform(selectedEntry);
    glm::vec3 translate, scale, skew;
    glm::vec4 perspective;
    glm::quat orientation;
    glm::decompose(objectLocal, scale, orientation, translate, skew, perspective);
    animatingObject->getTransformation()->setTransformations(translate, scale, glm::normalize(orientation));
    objectMoved = true;
}

void AnimationSequenceInterface::restoreOriginalTransformation() {
    animatingObject->getTransformation()->setTransformations(originalTransformation.getTranslateSingle(),
                                                             originalTransformation.getScaleSingle(),
                                                             originalTransformation.getOrientationSingle());
    objectMoved = true;
}

std::vector<int> AnimationSequenceInterface::getBoundaries() const {
    std::vector<int> boundaries;
    boundaries.reserve(sections.size() + 1);
    boundaries.push_back(startTime);
    for (const AnimationSequenceItem &item : sections) {
        boundaries.push_back(item.mFrameEnd);
    }
    return boundaries;
}

//a section's start is the previous one's end, so ImSequencer can't tell us which handle moved, only which boundary did.
//moving a whole bar changes two boundaries by the same amount, so the last changed one is the one to push from
void AnimationSequenceInterface::pushLaterSections(const std::vector<int> &boundariesBefore) {
    std::vector<int> boundariesAfter = getBoundaries();
    if (boundariesAfter.size() != boundariesBefore.size()) {
        return;//added or removed this frame, those shift the rest themselves
    }
    for (size_t i = boundariesAfter.size(); i-- > 0;) {
        int delta = boundariesAfter[i] - boundariesBefore[i];
        if (delta != 0) {
            for (size_t later = i; later < sections.size(); later++) {//boundary i + 1 is sections[i] end
                sections[later].mFrameEnd += delta;
            }
            break;
        }
    }
    //ImSequencer only keeps a start before its own end, a start dragged past the previous section's start would make keys go back in time
    boundariesAfter = getBoundaries();
    for (size_t i = 1; i < boundariesAfter.size(); i++) {
        if (boundariesAfter[i] < boundariesAfter[i - 1]) {
            startTime = boundariesBefore[0];
            for (size_t sectionIndex = 0; sectionIndex < sections.size(); sectionIndex++) {
                sections[sectionIndex].mFrameEnd = boundariesBefore[sectionIndex + 1];
            }
            return;
        }
    }
}

void AnimationSequenceInterface::Add(int type [[gnu::unused]]) {
    sections.push_back(createSection(sections.back().mFrameEnd + NEW_SECTION_LENGTH));//we don't allow removing last element, so back() is valid
    selectedEntry = sections.size() - 1;
}

void AnimationSequenceInterface::Duplicate(int index) {
    for (size_t i = index + 1; i < sections.size(); i++) {
        sections[i].mFrameEnd += NEW_SECTION_LENGTH;
    }
    sections.insert(sections.begin() + index + 1, createSection(sections[index].mFrameEnd + NEW_SECTION_LENGTH));
    selectedEntry = index + 1;
}

void AnimationSequenceInterface::insertSectionAtStart() {
    for (AnimationSequenceItem &item : sections) {
        item.mFrameEnd += NEW_SECTION_LENGTH;
    }
    sections.insert(sections.begin(), createSection(startTime + NEW_SECTION_LENGTH));
    selectedEntry = 0;
}

void AnimationSequenceInterface::Del(int index) {
    //don't allow deleting last element
    if(sections.size() <= 1) {
        return;
    }
    //next one takes over the motion, so everything after ends up where it did
    if ((size_t)index + 1 < sections.size()) {
        sections[index + 1].relativeTransform = sections[index].relativeTransform * sections[index + 1].relativeTransform;
    }
    sections.erase(sections.begin() + index);
    sectionDeleted = true;
}

AnimationCustom AnimationSequenceInterface::buildAnimationFromCurrentItems() const {
    std::shared_ptr<AnimationNode> animationNode = std::make_shared<AnimationNode>();
    pushMatrixToAnimationTime(animationNode, 0, glm::mat4(1.0f));
    if (startTime != 0) {
        pushMatrixToAnimationTime(animationNode, startTime, glm::mat4(1.0f));
    }
    //equal times are fine, the node picks the later key so a zero length section is a jump
    glm::mat4 endTransform(1.0f);
    std::vector<AnimationCustom::SectionName> sectionNames;
    for (const AnimationSequenceItem &item : sections) {
        endTransform = endTransform * item.relativeTransform;
        pushMatrixToAnimationTime(animationNode, item.mFrameEnd, endTransform);
        sectionNames.push_back(AnimationCustom::SectionName{item.mFrameEnd, item.name});
    }
    AnimationCustom builtAnimation(animationNameBuffer, animationNode, sections.back().mFrameEnd);
    builtAnimation.setSectionNames(sectionNames);
    return builtAnimation;
}

void AnimationSequenceInterface::pushMatrixToAnimationTime(std::shared_ptr<AnimationNode> animationNode, float time, const glm::mat4 &transform) const {
    glm::vec3 translate, scale, skew;
    glm::vec4 perspective;
    glm::quat orientation;
    glm::decompose(transform, scale, orientation, translate, skew, perspective);
    animationNode->translates.push_back(translate);
    animationNode->translateTimes.push_back(time);
    animationNode->scales.push_back(scale);
    animationNode->scaleTimes.push_back(time);
    animationNode->rotations.push_back(glm::normalize(orientation));
    animationNode->rotationTimes.push_back(time);
}

bool AnimationSequenceInterface::addAnimationSequencerToEditor(bool nameTaken, bool &finished, bool &savedAsNew, bool &cancelled) {
    finished = savedAsNew = cancelled = false;
    objectMoved = false;

    ImGui::Begin("Animation Definition");

    if (strcmp(animationNameBuffer,"") == 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("New animation name:");
        ImGui::PopStyleColor();
    } else {
        ImGui::Text("New animation name:");
    }
    ImGui::SameLine();
    //double # because I don't want to show it
    ImGui::InputText("##newAnimationNameField", animationNameBuffer, sizeof(animationNameBuffer), ImGuiInputTextFlags_CharsNoBlank);

    ImGui::InputInt("Frame count", &mFrameCount);
    //ImSequencer only adds at the end or after a row, before the selection check so the object moves this frame
    if (ImGui::Button("Add section at start")) {
        insertSectionAtStart();
    }

    sectionDeleted = false;
    std::vector<int> boundariesBefore = getBoundaries();
    Sequencer(this, NULL, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL );
    pushLaterSections(boundariesBefore);
    if (sectionDeleted) {
        //ImSequencer drops the selection on any delete that shifts it, keep it if the section survived
        for (size_t i = 0; i < sections.size(); i++) {
            if (sections[i].sectionID == selectedSectionID) {
                selectedEntry = i;
                break;
            }
        }
    }
    uint32_t newSelectedSectionID = NO_SECTION;
    if (selectedEntry >= 0 && selectedEntry < (int)sections.size()) {
        newSelectedSectionID = sections[selectedEntry].sectionID;
    } else {
        selectedEntry = -1;
    }
    if (newSelectedSectionID != selectedSectionID) {
        selectedSectionID = newSelectedSectionID;
        moveObjectToSelectedSection();
    }
    ImGui::Text("Every 60 frames is 1 second in game time");

    if (selectedEntry >= 0) {
        ImGui::InputText("Section name", sections[selectedEntry].name, sizeof(sections[selectedEntry].name));
        if (ImGui::Button("Return to start")) {
            sections[selectedEntry].relativeTransform = glm::inverse(getSectionEndTransform(selectedEntry - 1));
            moveObjectToSelectedSection();
        }
        ImGui::SameLine();
        ImGui::Text("Selected section ends at the original transform, for loops");
    }

    bool nameEmpty = strcmp(animationNameBuffer,"") == 0;
    if (nameTaken) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Another animation already has this name");
    }
    if (nameEmpty || nameTaken) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Finish");
        ImGui::PopStyleVar();
    } else if(ImGui::Button("Finish")) {
        finished = true;
    }

    if (editedAnimationIndex != NO_ANIMATION) {
        ImGui::SameLine();
        //the copy would save over the original's file
        if (nameEmpty || nameTaken || editedAnimationName == animationNameBuffer) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Save as new");
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Give it a new name first");
            }
        } else if (ImGui::Button("Save as new")) {
            savedAsNew = true;
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Cancel ")) {
        cancelled = true;
    }
    if(cancelled || finished || savedAsNew) {
        restoreOriginalTransformation();
        //since return to origin is done, invalidate the object so destructor doesn't mess with it
        animatingObject = nullptr;
    }
    ImGui::End();
    return objectMoved;
}
