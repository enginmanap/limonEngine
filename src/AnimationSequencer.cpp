//
// Created by engin on 8.06.2018.
//

#include "AnimationSequencer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
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
    for (const AnimationSequenceItem &item : sections) {
        endTransform = endTransform * item.relativeTransform;
        pushMatrixToAnimationTime(animationNode, item.mFrameEnd, endTransform);
    }
    return AnimationCustom(animationNameBuffer, animationNode, sections.back().mFrameEnd);
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

bool AnimationSequenceInterface::addAnimationSequencerToEditor(bool &finished, bool &cancelled) {
    finished = cancelled = false;
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

    if (strcmp(animationNameBuffer,"") != 0) {
        if(ImGui::Button("Finish")) {
            finished = true;
        }
    } else {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Finish");
        ImGui::PopStyleVar();
    }

    ImGui::SameLine();
    if(ImGui::Button("Cancel ")) {
        cancelled = true;
    }
    if(cancelled ||finished) {
        restoreOriginalTransformation();
        //since return to origin is done, invalidate the object so destructor doesn't mess with it
        animatingObject = nullptr;
    }
    ImGui::End();
    return objectMoved;
}
