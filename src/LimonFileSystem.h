//
// Created by engin on 14.10.2026.
//

#ifndef LIMONENGINE_LIMONFILESYSTEM_H
#define LIMONENGINE_LIMONFILESYSTEM_H

#include <cstdint>
#include <streambuf>
#include <string>
#include <vector>
#include <SDL3/SDL_filesystem.h>
#include <tinyxml2.h>

//every read of game data goes through here, so a package can sit under it without touching the callers
class LimonFileSystem {
public:
    struct DirectoryEntry {
        std::string name;
        bool isDirectory = false;
    };

    //cereal reads through an istream, this lets it read a buffer from readFile without another copy
    class MemoryStreamBuffer : public std::streambuf {
    public:
        explicit MemoryStreamBuffer(std::vector<uint8_t> &contents) {
            char *begin = reinterpret_cast<char *>(contents.data());
            setg(begin, begin, begin + contents.size());
        }
    };

    static std::string normalizePath(const std::string &path);

    static bool fileExists(const std::string &path);

    static bool isDirectory(const std::string &path);

    static bool readFile(const std::string &path, std::vector<uint8_t> &contents);

    static bool readFile(const std::string &path, std::string &contents);

    //returns XML_ERROR_FILE_NOT_FOUND like LoadFile does, callers tell a missing optional file apart by it.
    //The document has no error of its own when the read fails, print ErrorIDToName of the result instead of ErrorName
    static tinyxml2::XMLError loadXML(const std::string &path, tinyxml2::XMLDocument &document);

    //names only, callers build the full path from the one they passed, so asset paths keep the form they had
    static bool listDirectory(const std::string &path, std::vector<DirectoryEntry> &entries);

private:
    template<class Container>
    static bool readIntoContainer(const std::string &path, Container &contents);

    static SDL_EnumerationResult SDLCALL collectDirectoryEntry(void *userData, const char *directoryName, const char *fileName);
};


#endif //LIMONENGINE_LIMONFILESYSTEM_H
