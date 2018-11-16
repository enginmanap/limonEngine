//
// Created by engin on 16.11.2018.
//

#include "PlayerExtensionInterface.h"

std::map<std::string, PlayerExtensionInterface*(*)(LimonAPI*)>* PlayerExtensionInterface::extensionTypesMap;
