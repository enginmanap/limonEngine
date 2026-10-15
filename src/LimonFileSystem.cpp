//
// Created by engin on 14.10.2026.
//

#include "LimonFileSystem.h"

#include <cstring>
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

bool LimonFileSystem::fileExists(const std::string &path) {
    SDL_PathInfo pathInfo;
    if (!SDL_GetPathInfo(normalizePath(path).c_str(), &pathInfo)) {
        return false;
    }
    return pathInfo.type == SDL_PATHTYPE_FILE;
}

bool LimonFileSystem::isDirectory(const std::string &path) {
    SDL_PathInfo pathInfo;
    if (!SDL_GetPathInfo(normalizePath(path).c_str(), &pathInfo)) {
        return false;
    }
    return pathInfo.type == SDL_PATHTYPE_DIRECTORY;
}

template<class Container>
bool LimonFileSystem::readIntoContainer(const std::string &path, Container &contents) {
    contents.clear();
    SDL_IOStream *stream = SDL_IOFromFile(normalizePath(path).c_str(), "rb");
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

bool LimonFileSystem::readFile(const std::string &path, std::vector<uint8_t> &contents) {
    return readIntoContainer(path, contents);
}

bool LimonFileSystem::readFile(const std::string &path, std::string &contents) {
    return readIntoContainer(path, contents);
}

tinyxml2::XMLError LimonFileSystem::loadXML(const std::string &path, tinyxml2::XMLDocument &document) {
    document.Clear();
    std::string contents;
    if (!readFile(path, contents)) {
        return fileExists(path) ? tinyxml2::XML_ERROR_FILE_COULD_NOT_BE_OPENED : tinyxml2::XML_ERROR_FILE_NOT_FOUND;
    }
    return document.Parse(contents.data(), contents.size());
}

SDL_EnumerationResult SDLCALL LimonFileSystem::collectDirectoryEntry(void *userData, const char *directoryName [[gnu::unused]], const char *fileName) {
    std::vector<DirectoryEntry> *entries = static_cast<std::vector<DirectoryEntry> *>(userData);
    if (std::strcmp(fileName, ".") == 0 || std::strcmp(fileName, "..") == 0) {
        return SDL_ENUM_CONTINUE;
    }
    DirectoryEntry entry;
    entry.name = fileName;
    entries->push_back(entry);
    return SDL_ENUM_CONTINUE;
}

bool LimonFileSystem::listDirectory(const std::string &path, std::vector<DirectoryEntry> &entries) {
    entries.clear();
    std::string normalizedPath = normalizePath(path);
    if (!SDL_EnumerateDirectory(normalizedPath.c_str(), collectDirectoryEntry, &entries)) {
        entries.clear();
        return false;
    }
    for (DirectoryEntry &entry : entries) {
        entry.isDirectory = isDirectory(normalizedPath + "/" + entry.name);
    }
    return true;
}
