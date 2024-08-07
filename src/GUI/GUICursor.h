//
// Created by engin on 14.06.2018.
//

#ifndef LIMONENGINE_CURSOR_H
#define LIMONENGINE_CURSOR_H


#include "GUIImageBase.h"

class GUICursor: public GUIImageBase {
    bool hidden=false;
public:
    GUICursor(GraphicsInterface* graphicsWrapper,  std::shared_ptr<AssetManager> assetManager, const std::string &imageFile) : GUIImageBase(graphicsWrapper,
                                                                                                                    assetManager,
                                                                                                                    imageFile) {}

public:

    void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram, uint32_t lodLevel) override;

    bool isHidden() {
        return hidden;
    }

    void hide() {
        this->hidden = true;
    }

    void unhide() {
        this->hidden = false;
    }
};


#endif //LIMONENGINE_CURSOR_H
