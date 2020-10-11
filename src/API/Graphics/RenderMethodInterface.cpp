//
// Created by engin on 23.09.2019.
//

#include "RenderMethodInterface.h"

std::map<std::string, RenderMethodInterface*(*)(GraphicsInterface*)>* RenderMethodInterface::typeMap;
