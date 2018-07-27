//
// Created by engin on 14.06.2018.
//

#include "GUICursor.h"

void GUICursor::render() {
    if(!this->hidden) {
        GUIImageBase::render();
    }
}
