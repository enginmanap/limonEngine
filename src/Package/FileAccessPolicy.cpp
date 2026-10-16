//
// Created by engin on 15.10.2026.
//

#include "FileAccessPolicy.h"

#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include "WorldReferenceScanner.h"
#include "../LimonFileSystem.h"
#include "limonAPI/TriggerInterface.h"
#include "limonAPI/ActorInterface.h"
#include "limonAPI/PlayerExtensionInterface.h"
#include "limonAPI/CameraExtensionInterface.h"
#include "limonAPI/Graphics/RenderMethodInterface.h"

static std::mutex worldReferencesMutex;
static std::map<std::string, std::set<std::string>> worldReferences;//key is the normalized world file

void FileAccessPolicy::addWorld(const std::string &worldFile, const tinyxml2::XMLDocument &document) {
    WorldReferenceScanner::References references;
    WorldReferenceScanner::scanDocument(worldFile, document, {}, references);
    std::lock_guard<std::mutex> lock(worldReferencesMutex);
    worldReferences[LimonFileSystem::normalizePath(worldFile)] = references.files;
}

void FileAccessPolicy::removeWorld(const std::string &worldFile) {
    std::lock_guard<std::mutex> lock(worldReferencesMutex);
    worldReferences.erase(LimonFileSystem::normalizePath(worldFile));
}

bool FileAccessPolicy::isInList(const std::vector<std::string> &packFileAccessList, const std::string &normalizedPath) {
    for (const std::string &entry : packFileAccessList) {
        std::string normalizedEntry = LimonFileSystem::normalizePath(entry);
        if (!entry.empty() && (entry.back() == '/' || entry.back() == '\\')) {
            if (normalizedPath == normalizedEntry || normalizedPath.compare(0, normalizedEntry.size() + 1, normalizedEntry + "/") == 0) {
                return true;
            }
        } else if (normalizedPath == normalizedEntry) {
            return true;
        }
    }
    return false;
}

//read from the registries on every call, Python registers again for each world interpreter and a cache would go stale
bool FileAccessPolicy::isDeclared(const std::string &normalizedPath) {
    for (const std::string &typeName : TriggerInterface::getTriggerNames()) {
        if (isInList(TriggerInterface::getRegisteredPackFileAccessList(typeName), normalizedPath)) {
            return true;
        }
    }
    for (const std::string &typeName : ActorInterface::getActorNames()) {
        if (isInList(ActorInterface::getRegisteredPackFileAccessList(typeName), normalizedPath)) {
            return true;
        }
    }
    for (const std::string &typeName : PlayerExtensionInterface::getExtensionNames()) {
        if (isInList(PlayerExtensionInterface::getRegisteredPackFileAccessList(typeName), normalizedPath)) {
            return true;
        }
    }
    for (const std::string &typeName : CameraExtensionInterface::getExtensionNames()) {
        if (isInList(CameraExtensionInterface::getRegisteredPackFileAccessList(typeName), normalizedPath)) {
            return true;
        }
    }
    for (const std::string &typeName : RenderMethodInterface::getRenderMethodNames()) {
        if (isInList(RenderMethodInterface::getRegisteredPackFileAccessList(typeName), normalizedPath)) {
            return true;
        }
    }
    return false;
}

bool FileAccessPolicy::isAllowed(const std::string &path) {
    std::string normalizedPath = LimonFileSystem::normalizePath(path);
    if (normalizedPath.compare(0, 5, "Data/") != 0) {
        return true;//Engine/ ships with the engine, and anything outside Data/ is never packed
    }
    {
        std::lock_guard<std::mutex> lock(worldReferencesMutex);
        for (const std::pair<const std::string, std::set<std::string>> &world : worldReferences) {
            if (world.second.count(normalizedPath) != 0) {
                return true;
            }
        }
    }
    if (isDeclared(normalizedPath)) {
        return true;
    }
    std::cerr << "Refused access to " << path << ": no loaded world references it and no registered type lists it. "
              << "Add it to the getPackFileAccessList (Python: get_pack_file_access_list) of the type that uses it." << std::endl;
    return false;
}
