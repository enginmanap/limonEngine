//
// Created by engin on 18.05.2026.
//

#ifndef LIMONENGINE_EDITORRENDERABLE_H
#define LIMONENGINE_EDITORRENDERABLE_H

#include "ImGuiRequest.h"
#include "ImGuiResult.h"

class EditorRenderable {
public:
    virtual ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) = 0;
    virtual ~EditorRenderable() = default;
};

#endif //LIMONENGINE_EDITORRENDERABLE_H