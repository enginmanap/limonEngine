//
// Created by engin on 8.06.2018.
//

#include "AnimationSequencer.h"
#include "Renderable.h"
#include "../libs/ImGui/imgui.h"
#include "Assets/Animations/AnimationCustom.h"

AnimationSequenceInterface::AnimationSequenceInterface(Renderable* animatingObject) :
        originalTransformation(*animatingObject->getTransformation()), animatingObject(animatingObject) {
    sections.push_back(AnimationSequenceInterface::AnimationSequenceItem{ 30, Transformation() });//put one element to sections by default
}

AnimationSequenceInterface::~AnimationSequenceInterface(){
    //put object back to original position
    if(animatingObject != nullptr) { //if cancelled or finalized, this is already done, and animating object is null
        animatingObject->getTransformation()->setTranslate(originalTransformation.getTranslate());
        animatingObject->getTransformation()->setScale(originalTransformation.getScale());
        animatingObject->getTransformation()->setOrientation(originalTransformation.getOrientation());
    }
}

void AnimationSequenceInterface::Duplicate(int index) {
    //before duplication, set current transform to the item.
    setTransform(index);
    //the plan is, push the next element, and add this in between
    sections.insert(sections.begin() + index, sections[index]);
    selectedEntry = index+1;
    //get the length of the section we are duplicating
    uint32_t length;
    if(index == 0) {
        length = sections[index].mFrameEnd - startTime;
    } else {
        length = sections[index].mFrameEnd - sections[index-1].mFrameEnd;
    }

    sections[index+1].mFrameEnd = sections[index].mFrameEnd+length;

    // now we pushed the element after index 1 more, so it is +2.
    uint32_t indexPlus2 = index + 2; //this is just to prevent the warning signed/unsigned comparison
    if(sections.size() > indexPlus2) {
        //move all the elements after
        for(size_t i = indexPlus2; i < sections.size(); i++) {
            sections[i].mFrameEnd += length;
        }
    }
}

void AnimationSequenceInterface::Add(int type __attribute((unused))) {
    setTransform(selectedEntry);
    Transformation tempTr = sections[sections.size()-1].transformation;//we don't allow removing last element, so this is valid.
    sections.push_back(AnimationSequenceInterface::AnimationSequenceItem{ sections[sections.size()-1].mFrameEnd+60,  tempTr});
    selectedEntry = sections.size()-1;//select last element
}

void AnimationSequenceInterface::setTransform(int32_t indexToSet) {
    if(indexToSet == -1) {
        //no element is selected, this transform is not going to be set to anything;
        return;
    }
    glm::vec3 translate, scale;
    glm::quat rotation;
    originalTransformation.getDifferenceStacked(*animatingObject->getTransformation(), translate, scale, rotation);
    AnimationSequenceInterface::AnimationSequenceItem &item = sections[indexToSet];
    item.transformation.setTransformationsNotPropagate(translate,rotation,scale);
}

AnimationCustom* AnimationSequenceInterface::buildAnimationFromCurrentItems() {
    //first, delete old one
    delete animationInProgress;
    //create new ones
    std::shared_ptr<AnimationNode> animationNode = std::make_shared<AnimationNode>();
    animationInProgress = new AnimationCustom(animationNameBuffer, animationNode, 0);// we don't have a duration yet. it will be determined afterwards

    if(sections.size() == 0) {
        pushIdentityToAnimationTime(animationInProgress->animationNode, 0);
        return animationInProgress;
    }

    //add starting position position
    fillAnimationFromItem(0, animationInProgress, Transformation(), true);//for the first element, we push identity transform
    int maxDuration = sections[0].mFrameEnd;

    //now iterate over the list, combining the animations

    /**
     * lets look at what we have here
     * 1) We have first items animation that is in animationBase
     * 2) we have an empty animation in progress.
     *
     * what we want to do?
     * 1) we will sample both animation base, and item animation. we need all the key frame times
     * 2) for each and every frame we will sample both animation, combine the result and save the result in animationInProgress
     * check if sample time is not with in animation time, then for before animation assume 0, after animation assume last.
     *
     * after this is done, copy animationInProgress to base, clear it, iterate.
     *
     */

    AnimationCustom* animationBase = nullptr;


//        AnimationNode* animationNodeInProgress = animationInProgress->animationNode;

    for(size_t i = 1; i < sections.size(); i++) {
        delete animationBase;
        animationBase = new AnimationCustom(*animationInProgress);
        delete animationInProgress;
        auto animationNode = std::make_shared<AnimationNode>();
        animationInProgress = new AnimationCustom(animationNameBuffer, animationNode, maxDuration);

        //setup done, build animation for current item.
        AnimationSequenceItem& item = sections[i];
        auto animationNode2 = std::make_shared<AnimationNode>();
        AnimationCustom* itemsAnimation = new AnimationCustom("item", animationNode2, 0);
        fillAnimationFromItem(i, itemsAnimation, sections[i-1].transformation, false);

        //sample both, join
        //how to sample both? we know they both start from 0, just get which one is shorter, sample until the shorter end.
        float minDuration = std::fmin(animationBase->getDuration(), itemsAnimation->getDuration());

        uint32_t baseIndex=0, itemIndex=0;
        while(animationBase->animationNode->rotationTimes[baseIndex] <= minDuration &&
              itemsAnimation->animationNode->rotationTimes[itemIndex] <= minDuration ) {
            float time;
            if(animationBase->animationNode->rotationTimes[baseIndex] ==
               itemsAnimation->animationNode->rotationTimes[itemIndex]) {
                //if they are both equal do this
                time = animationBase->animationNode->rotationTimes[baseIndex];
                itemIndex++;
                baseIndex++;
            } else {
                //they are not the same;
                if(animationBase->animationNode->rotationTimes[baseIndex] <
                   itemsAnimation->animationNode->rotationTimes[itemIndex]) {
                    //if base animation has a time smaller at the moment
                    time = animationBase->animationNode->rotationTimes[baseIndex];
                    baseIndex++;
                } else {
                    //if item animation time is smaller
                    time = itemsAnimation->animationNode->rotationTimes[itemIndex];
                    itemIndex++;
                }
            }
            Transformation totalTr;
            Transformation animTr;
            animationBase->calculateTransform("", time, totalTr);
            itemsAnimation->calculateTransform("", time, animTr);
            totalTr.combine(animTr);
            pushTransformToAnimationTime(animationInProgress->animationNode, time, totalTr);
        }
        //now we have the short part fully joined, we need to repeat the short ones last transform
        AnimationCustom* longerAnimation;
        Transformation shortAnimationLastTr;
        uint32_t longerStartIndex;

        if(itemsAnimation->getDuration() > animationBase->getDuration()) {
            longerAnimation = itemsAnimation;
            animationBase->calculateTransform("", animationBase->getDuration(), shortAnimationLastTr);
            longerStartIndex = itemIndex;
        } else {
            longerAnimation = animationBase;
            itemsAnimation->calculateTransform("", itemsAnimation->getDuration(), shortAnimationLastTr);
            longerStartIndex = baseIndex;
        }
        for (size_t j = longerStartIndex; j < longerAnimation->animationNode->rotationTimes.size(); ++j) {
            float time = longerAnimation->animationNode->rotationTimes[j];
            Transformation longerTr;
            longerAnimation->calculateTransform("", time, longerTr);
            longerTr.combine(shortAnimationLastTr);
            pushTransformToAnimationTime(animationInProgress->animationNode, time, longerTr);
        }

        maxDuration = std::max(maxDuration, item.mFrameEnd);
        animationInProgress->duration = maxDuration;
    }

    return animationInProgress;
}

/**
 * We should calculate the difference for stacking, but only first one is actually stacked, the rest is added to first transform, so the useStacked flag. We should have used index, but I want this to be easy to understand
 *
 * @param itemIndex
 * @param animationToFill
 * @param sourceTransform
 * @param useStacked
 */
void AnimationSequenceInterface::fillAnimationFromItem(uint32_t itemIndex, AnimationCustom *animationToFill, const Transformation& sourceTransform, bool useStacked) const {
    AnimationSequenceItem item = sections[itemIndex];
    int animationStartTime = this->startTime;
    if(itemIndex > 0) {
        animationStartTime = sections[itemIndex -1].mFrameEnd;
    }
    assert(animationToFill != nullptr);
    if(animationStartTime != 0) {
        //start with identity, if we are not already
        pushIdentityToAnimationTime(animationToFill->animationNode, 0);
    }

    pushIdentityToAnimationTime(animationToFill->animationNode, animationStartTime);

    glm::vec3 translate, scale;
    glm::quat rotation;
    if(useStacked) {
        sourceTransform.getDifferenceStacked(item.transformation, translate, scale, rotation);
    } else {
        sourceTransform.getDifferenceAddition(item.transformation, translate, scale, rotation);
    }

    Transformation difference;
    difference.setTransformationsNotPropagate(translate,rotation,scale);
    pushTransformToAnimationTime(animationToFill->animationNode, item.mFrameEnd, difference);
    animationToFill->duration = item.mFrameEnd;
}

void AnimationSequenceInterface::pushTransformToAnimationTime(std::shared_ptr<AnimationNode> animationNode, float time, const Transformation &transformation) const {
    animationNode->translates.push_back(transformation.getTranslate());
    animationNode->translateTimes.push_back(time);
    animationNode->scales.push_back(transformation.getScale());
    animationNode->scaleTimes.push_back(time);
    animationNode->rotations.push_back(transformation.getOrientation());
    animationNode->rotationTimes.push_back(time);
}

void AnimationSequenceInterface::pushIdentityToAnimationTime(std::shared_ptr<AnimationNode>animationNode, float time) const {
    animationNode->translates.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    animationNode->translateTimes.push_back(time);
    animationNode->scales.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
    animationNode->scaleTimes.push_back(time);
    animationNode->rotations.push_back(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    animationNode->rotationTimes.push_back(time);
}

void AnimationSequenceInterface::addAnimationSequencerToEditor(bool &finished, bool &cancelled) {
    finished = cancelled = false;

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

    int oldSelectedEntry = selectedEntry;
    bool isSelectionUpdated = Sequencer(this, NULL, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL );
    ImGui::Text("Every 60 frames is 1 second in game time");
    if(isSelectionUpdated) {
        if (selectedEntry != oldSelectedEntry) {
            //set the current position of the attached model to old selected entry, then, move the model to new selected entries transformation
            setTransform(oldSelectedEntry);

            //we set the transformation to item. now move the object:
            if (selectedEntry == -1) {
                //means nothing is selected, can happen when sequence removed.
                animatingObject->getTransformation()->setTranslate(originalTransformation.getTranslate());
                animatingObject->getTransformation()->setScale(originalTransformation.getScale());
                animatingObject->getTransformation()->setOrientation(originalTransformation.getOrientation());
            } else {
                Transformation itemTransformation(originalTransformation);
                itemTransformation.combine(sections[selectedEntry].transformation);
                //If there is a parent, they share the parent so single is appropriate. If no parent, single works as normal
                animatingObject->getTransformation()->setTranslate(itemTransformation.getTranslateSingle());
                animatingObject->getTransformation()->setScale(itemTransformation.getScaleSingle());
                animatingObject->getTransformation()->setOrientation(itemTransformation.getOrientationSingle());
            }
        }
    }


    if (strcmp(animationNameBuffer,"") != 0) {
        if(ImGui::Button("Finish")) {
            setTransform(selectedEntry);
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
        //Single works both parented and not parented transforms.
        animatingObject->getTransformation()->setTranslate(originalTransformation.getTranslateSingle());
        animatingObject->getTransformation()->setScale(originalTransformation.getScaleSingle());
        animatingObject->getTransformation()->setOrientation(originalTransformation.getOrientationSingle());
        //since return to origin is done, invalidate the object so destructor doesn't mess with it
        animatingObject = nullptr;
    }
    ImGui::End();
}
