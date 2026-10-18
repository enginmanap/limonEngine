//
// Created by engin on 18.10.2026.
//

#ifndef LIMONENGINE_PACKAGINGCALLBACKS_H
#define LIMONENGINE_PACKAGINGCALLBACKS_H

#include <functional>
#include <string>

class PackageBuilder;

//GameEngine owns the packer, and it only exists while a pack runs. The editor starts and finishes one through these
struct PackagingCallbacks {
    //prepares on the calling thread, which must be the main one, and starts the worker. nullptr and the reason when it refuses.
    //An empty startingWorld packs from Release.xml's StartingWorld
    std::function<PackageBuilder *(const std::string &outputFile, const std::string &startingWorld, std::string &refusalReason)> start;
    //joins the worker and frees the packer, once its progress says DONE. Returns whether the pack succeeded
    std::function<bool()> finish;
};

#endif //LIMONENGINE_PACKAGINGCALLBACKS_H
