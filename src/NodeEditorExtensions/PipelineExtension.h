//
// Created by engin on 15.04.2019.
//

#ifndef LIMONENGINE_PIPELINEEXTENSION_H
#define LIMONENGINE_PIPELINEEXTENSION_H


#include <nodeGraph/src/EditorExtension.h>
#include <GLHelper.h>

class PipelineExtension : public EditorExtension {
    std::map<std::string, std::shared_ptr<GLHelper::Texture>> usedTextures;
    GLHelper* glHelper = nullptr;
public:

    PipelineExtension(GLHelper* glHelper) : glHelper(glHelper) {}
    void drawDetailPane() override;

    static bool getNameOfTexture(void* data, int index, const char** outText);
};


#endif //LIMONENGINE_PIPELINEEXTENSION_H
