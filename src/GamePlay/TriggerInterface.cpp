//
// Created by engin on 26.05.2018.
//

#include "TriggerInterface.h"
#include <iostream>

#include "SDKSerializer.h"

std::map<std::string, TriggerInterface*(*)(LimonAPI*)>* TriggerInterface::typeMap;