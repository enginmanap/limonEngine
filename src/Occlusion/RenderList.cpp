//
// Created by Engin Manap on 21/12/2024.
//

#include "RenderList.h"

#include <iostream>

void RenderList::exitOnWrongBuildState(const char *operation, const char *problem) {
    std::cerr << "Render list " << operation << problem << std::endl;
    exit(-1);
}
