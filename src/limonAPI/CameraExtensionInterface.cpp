//
// Created by engin on 2026.
//

#include "CameraExtensionInterface.h"

std::map<std::string, CameraExtensionInterface*(*)(LimonAPI*)>* CameraExtensionInterface::extensionTypesMap;
