//
// Created by engin on 14/01/2024.
//

#ifndef IMGUIRESULT_H
#define IMGUIRESULT_H

#include <string>

/**
 * Since the world is not passed with ImGui request, changes to world must be returned using this struct
 */
struct ImGuiResult {
    bool addAI = false;
    bool removeAI = false;
    bool updated = false;
    bool remove = false; //If removal requested
    bool materialChanged = false;
    bool recalculateTranslateForOnTop = false;
    std::string actorTypeName;
};



#endif //IMGUIRESULT_H
