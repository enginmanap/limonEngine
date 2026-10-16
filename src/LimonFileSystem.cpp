//
// Created by engin on 14.10.2026.
//

#include "LimonFileSystem.h"

#include <cstring>
#include <iostream>
#include <set>
#include <physfs.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>

std::string LimonFileSystem::normalizePath(const std::string &path) {
    std::string slashedPath = path;
    for (char &character : slashedPath) {
        if (character == '\\') {
            character = '/';
        }
    }
    std::string root;
    size_t segmentStart = 0;
    if (!slashedPath.empty() && slashedPath[0] == '/') {
        root = "/";
        segmentStart = 1;
    } else if (slashedPath.size() >= 2 && slashedPath[1] == ':') {
        root = slashedPath.substr(0, 2);
        segmentStart = 2;
        if (slashedPath.size() >= 3 && slashedPath[2] == '/') {
            root += "/";
            segmentStart = 3;
        }
    }

    std::vector<std::string> segments;
    while (segmentStart <= slashedPath.size()) {
        size_t segmentEnd = slashedPath.find('/', segmentStart);
        if (segmentEnd == std::string::npos) {
            segmentEnd = slashedPath.size();
        }
        std::string segment = slashedPath.substr(segmentStart, segmentEnd - segmentStart);
        segmentStart = segmentEnd + 1;
        if (segment.empty() || segment == ".") {
            continue;
        }
        if (segment == "..") {
            if (!segments.empty() && segments.back() != "..") {
                segments.pop_back();
            } else if (root.empty()) {
                segments.push_back(segment);//a relative path may climb out of the working directory, a rooted one can't
            }
            continue;
        }
        segments.push_back(segment);
    }

    std::string normalizedPath = root;
    for (size_t segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex) {
        if (segmentIndex != 0) {
            normalizedPath += "/";
        }
        normalizedPath += segments[segmentIndex];
    }
    if (normalizedPath.empty()) {
        return ".";
    }
    return normalizedPath;
}

//mount points, never visible outside this file. Set once in initialize, before any loading thread exists
static const std::string ENGINE_LAYER = "engine";
static const std::string PACKAGE_LAYER = "package";
static const std::string DATA_LAYER = "data";
static bool packageMounted = false;
static bool dataMounted = false;

static PHYSFS_EnumerateCallbackResult collectLayerEntry(void *userData, const char *directoryName [[gnu::unused]], const char *fileName) {
    static_cast<std::vector<std::string> *>(userData)->push_back(fileName);
    return PHYSFS_ENUM_OK;
}

static std::string lastPhysfsError() {
    return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}

bool LimonFileSystem::initialize(const char *argv0) {
    if (!PHYSFS_init(argv0)) {
        std::cerr << "File system could not start: " << lastPhysfsError() << std::endl;
        return false;
    }
    PHYSFS_permitSymbolicLinks(1);//PhysicsFS refuses symlinked files under a mount by default, loose files never did
    if (!PHYSFS_mount("./Engine", ENGINE_LAYER.c_str(), 1)) {
        std::cerr << "Could not mount ./Engine: " << lastPhysfsError() << std::endl;
        return false;
    }
    SDL_PathInfo pathInfo;
    if (SDL_GetPathInfo("./Data.pak", &pathInfo)) {
        if (!PHYSFS_mount("./Data.pak", PACKAGE_LAYER.c_str(), 1)) {
            std::cerr << "Could not mount ./Data.pak: " << lastPhysfsError() << std::endl;
            return false;
        }
        packageMounted = true;
        std::cout << "Mounted ./Data.pak, loose files in ./Data override it" << std::endl;
    }
    if (SDL_GetPathInfo("./Data", &pathInfo) && pathInfo.type == SDL_PATHTYPE_DIRECTORY) {
        if (!PHYSFS_mount("./Data", DATA_LAYER.c_str(), 1)) {
            std::cerr << "Could not mount ./Data: " << lastPhysfsError() << std::endl;
            return false;
        }
        dataMounted = true;
    }
    return true;
}

bool LimonFileSystem::isPackageMounted() {
    return packageMounted;
}

bool LimonFileSystem::isLooseDataFile(const std::string &path) {
    std::vector<std::string> layerPaths;
    if (!dataMounted || !resolveLayers(normalizePath(path), layerPaths) || layerPaths.empty()) {
        return false;
    }
    //Data/ paths resolve to the loose layer first when it is mounted
    return layerPaths[0].compare(0, DATA_LAYER.size(), DATA_LAYER) == 0 && isLayerFile(layerPaths[0]);
}

bool LimonFileSystem::resolveLayers(const std::string &normalizedPath, std::vector<std::string> &layerPaths) {
    layerPaths.clear();
    size_t slashPosition = normalizedPath.find('/');
    std::string firstSegment = normalizedPath.substr(0, slashPosition);
    std::string remainingPath = slashPosition == std::string::npos ? "" : "/" + normalizedPath.substr(slashPosition + 1);
    if (firstSegment == "Engine") {
        layerPaths.push_back(ENGINE_LAYER + remainingPath);//nothing overrides the engine's own files
        return true;
    }
    if (firstSegment == "Data") {
        if (dataMounted) {
            layerPaths.push_back(DATA_LAYER + remainingPath);
        }
        if (packageMounted) {
            layerPaths.push_back(PACKAGE_LAYER + remainingPath);
        }
        return true;
    }
    return false;
}

bool LimonFileSystem::isLayerFile(const std::string &layerPath) {
    PHYSFS_Stat stat;
    return PHYSFS_stat(layerPath.c_str(), &stat) && stat.filetype == PHYSFS_FILETYPE_REGULAR;
}

bool LimonFileSystem::isLayerDirectory(const std::string &layerPath) {
    PHYSFS_Stat stat;
    return PHYSFS_stat(layerPath.c_str(), &stat) && stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

bool LimonFileSystem::fileExists(const std::string &path) {
    std::string normalizedPath = normalizePath(path);
    std::vector<std::string> layerPaths;
    if (!resolveLayers(normalizedPath, layerPaths)) {
        SDL_PathInfo pathInfo;
        return SDL_GetPathInfo(normalizedPath.c_str(), &pathInfo) && pathInfo.type == SDL_PATHTYPE_FILE;
    }
    for (const std::string &layerPath : layerPaths) {
        if (isLayerFile(layerPath)) {
            return true;
        }
    }
    return false;
}

bool LimonFileSystem::isDirectory(const std::string &path) {
    std::string normalizedPath = normalizePath(path);
    std::vector<std::string> layerPaths;
    if (!resolveLayers(normalizedPath, layerPaths)) {
        SDL_PathInfo pathInfo;
        return SDL_GetPathInfo(normalizedPath.c_str(), &pathInfo) && pathInfo.type == SDL_PATHTYPE_DIRECTORY;
    }
    for (const std::string &layerPath : layerPaths) {
        if (isLayerDirectory(layerPath)) {
            return true;
        }
    }
    return false;
}

template<class Container>
bool LimonFileSystem::readFromLayer(const std::string &layerPath, Container &contents) {
    contents.clear();
    PHYSFS_File *file = PHYSFS_openRead(layerPath.c_str());
    if (file == nullptr) {
        SDL_SetError("Couldn't open %s: %s", layerPath.c_str(), lastPhysfsError().c_str());//callers print SDL_GetError, like they did for IMG_Load
        return false;
    }
    PHYSFS_sint64 fileLength = PHYSFS_fileLength(file);
    if (fileLength < 0) {
        SDL_SetError("Couldn't size %s: %s", layerPath.c_str(), lastPhysfsError().c_str());
        PHYSFS_close(file);
        return false;
    }
    contents.resize(static_cast<size_t>(fileLength));
    PHYSFS_sint64 readLength = 0;
    if (!contents.empty()) {
        readLength = PHYSFS_readBytes(file, &contents[0], static_cast<PHYSFS_uint64>(fileLength));
    }
    PHYSFS_close(file);
    if (readLength != fileLength) {
        SDL_SetError("Couldn't read %s: %s", layerPath.c_str(), lastPhysfsError().c_str());
        contents.clear();
        return false;
    }
    return true;
}

template<class Container>
bool LimonFileSystem::readFromDisk(const std::string &normalizedPath, Container &contents) {
    contents.clear();
    SDL_IOStream *stream = SDL_IOFromFile(normalizedPath.c_str(), "rb");
    if (stream == nullptr) {
        return false;
    }
    Sint64 fileSize = SDL_GetIOSize(stream);
    if (fileSize < 0) {
        SDL_CloseIO(stream);
        return false;
    }
    contents.resize(static_cast<size_t>(fileSize));
    size_t readSize = 0;
    if (!contents.empty()) {
        readSize = SDL_ReadIO(stream, &contents[0], contents.size());
    }
    SDL_CloseIO(stream);
    if (readSize != contents.size()) {
        contents.clear();
        return false;
    }
    return true;
}

template<class Container>
bool LimonFileSystem::readIntoContainer(const std::string &path, Container &contents) {
    contents.clear();
    std::string normalizedPath = normalizePath(path);
    std::vector<std::string> layerPaths;
    if (!resolveLayers(normalizedPath, layerPaths)) {
        return readFromDisk(normalizedPath, contents);
    }
    for (const std::string &layerPath : layerPaths) {
        if (isLayerFile(layerPath)) {
            return readFromLayer(layerPath, contents);//a layer that has the file but fails to read it must not let a lower one answer
        }
    }
    SDL_SetError("Couldn't open %s: not found", normalizedPath.c_str());
    return false;
}

bool LimonFileSystem::readFile(const std::string &path, std::vector<uint8_t> &contents) {
    return readIntoContainer(path, contents);
}

bool LimonFileSystem::readFile(const std::string &path, std::string &contents) {
    return readIntoContainer(path, contents);
}

std::vector<std::string> LimonFileSystem::readFileFromAllLayers(const std::string &path) {
    std::vector<std::string> layerContents;
    std::string normalizedPath = normalizePath(path);
    std::vector<std::string> layerPaths;
    if (!resolveLayers(normalizedPath, layerPaths)) {
        std::string contents;
        if (readFromDisk(normalizedPath, contents)) {
            layerContents.push_back(contents);
        }
        return layerContents;
    }
    for (std::vector<std::string>::reverse_iterator layerPath = layerPaths.rbegin(); layerPath != layerPaths.rend(); ++layerPath) {
        std::string contents;
        if (isLayerFile(*layerPath) && readFromLayer(*layerPath, contents)) {
            layerContents.push_back(contents);
        }
    }
    return layerContents;
}

tinyxml2::XMLError LimonFileSystem::loadXML(const std::string &path, tinyxml2::XMLDocument &document) {
    document.Clear();
    std::string contents;
    if (!readFile(path, contents)) {
        return fileExists(path) ? tinyxml2::XML_ERROR_FILE_COULD_NOT_BE_OPENED : tinyxml2::XML_ERROR_FILE_NOT_FOUND;
    }
    return document.Parse(contents.data(), contents.size());
}

SDL_EnumerationResult SDLCALL LimonFileSystem::collectDiskEntry(void *userData, const char *directoryName [[gnu::unused]], const char *fileName) {
    std::vector<DirectoryEntry> *entries = static_cast<std::vector<DirectoryEntry> *>(userData);
    if (std::strcmp(fileName, ".") == 0 || std::strcmp(fileName, "..") == 0) {
        return SDL_ENUM_CONTINUE;
    }
    DirectoryEntry entry;
    entry.name = fileName;
    entries->push_back(entry);
    return SDL_ENUM_CONTINUE;
}

bool LimonFileSystem::listDiskDirectory(const std::string &normalizedPath, std::vector<DirectoryEntry> &entries) {
    if (!SDL_EnumerateDirectory(normalizedPath.c_str(), collectDiskEntry, &entries)) {
        entries.clear();
        return false;
    }
    for (DirectoryEntry &entry : entries) {
        SDL_PathInfo pathInfo;
        entry.isDirectory = SDL_GetPathInfo((normalizedPath + "/" + entry.name).c_str(), &pathInfo) && pathInfo.type == SDL_PATHTYPE_DIRECTORY;
    }
    return true;
}

bool LimonFileSystem::listDirectory(const std::string &path, std::vector<DirectoryEntry> &entries) {
    entries.clear();
    std::string normalizedPath = normalizePath(path);
    std::vector<std::string> layerPaths;
    if (!resolveLayers(normalizedPath, layerPaths)) {
        return listDiskDirectory(normalizedPath, entries);
    }
    bool directoryFound = false;
    std::set<std::string> listedNames;
    for (const std::string &layerPath : layerPaths) {
        if (!isLayerDirectory(layerPath)) {
            continue;
        }
        directoryFound = true;
        std::vector<std::string> names;
        PHYSFS_enumerate(layerPath.c_str(), collectLayerEntry, &names);
        for (const std::string &name : names) {
            if (!listedNames.insert(name).second) {
                continue;//a higher layer already listed it, and that one is what a read gets
            }
            DirectoryEntry entry;
            entry.name = name;
            entry.isDirectory = isLayerDirectory(layerPath + "/" + name);
            entries.push_back(entry);
        }
    }
    return directoryFound;
}
