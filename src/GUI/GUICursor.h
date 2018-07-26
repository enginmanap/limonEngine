//
// Created by engin on 14.06.2018.
//

#ifndef LIMONENGINE_CURSOR_H
#define LIMONENGINE_CURSOR_H


#include "GUIImage.h"

class GUICursor: public GUIImage {
public:
    GUICursor(GLHelper *glHelper, AssetManager *assetManager, const std::string &imageFile) : GUIImage(glHelper,
                                                                                                       assetManager,
                                                                                                       imageFile) {}

public:

};


#endif //LIMONENGINE_CURSOR_H
