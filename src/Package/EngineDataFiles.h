//
// Created by engin on 15.10.2026.
//

#ifndef LIMONENGINE_ENGINEDATAFILES_H
#define LIMONENGINE_ENGINEDATAFILES_H

#include <string>
#include <vector>

//Data/ files the engine itself reads by a fixed name. No world or extension names them, so the packer ships these by this list
class EngineDataFiles {
public:
    static const std::string RELEASE;
    static const std::string USER_OPTIONS;
    static const std::string INPUT_BINDINGS;
    static const std::string DEFAULT_FONT;
    static const std::string DEBUG_FONT;
    static const std::string CROSSHAIR;
    static const std::string PACKAGE_MANIFEST;//inside a package only, so not in getAll: the packer writes it, never packs it

    static std::vector<std::string> getAll();
};


#endif //LIMONENGINE_ENGINEDATAFILES_H
