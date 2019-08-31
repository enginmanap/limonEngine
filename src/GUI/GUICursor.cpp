//
// Created by engin on 14.06.2018.
//

#include "GUICursor.h"

void GUICursor::renderWithProgram(std::shared_ptr<GLSLProgram> renderProgram){
    if(!this->hidden) {
        GUIImageBase::renderWithProgram(renderProgram);
    }
}
