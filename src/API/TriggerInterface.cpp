//
// Created by engin on 26.05.2018.
//

#include "TriggerInterface.h"
#include <iostream>

std::map<std::string, TriggerInterface*(*)(LimonAPI*)>* TriggerInterface::typeMap;