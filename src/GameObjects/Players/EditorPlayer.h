//
// Created by engin on 27.02.2018.
//

#ifndef LIMONENGINE_EDITORPLAYER_H
#define LIMONENGINE_EDITORPLAYER_H

#include "FreeCursorPlayer.h"

class InputHandler;

class EditorPlayer : public FreeCursorPlayer {
    InputHandler* inputHandler;
    void rotateFree(float xChange, float yChange);
public:
    EditorPlayer(OptionsUtil::Options *options, GUIRenderable *cursor, const glm::vec3 &position,
                 const glm::vec3 &lookDirection, InputHandler* inputHandler);

    void processInput(const InputStates &inputState, long time) override;
};

#endif //LIMONENGINE_EDITORPLAYER_H
