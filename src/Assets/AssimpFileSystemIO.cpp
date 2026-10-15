//
// Created by engin on 14.10.2026.
//

#include "AssimpFileSystemIO.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include "../LimonFileSystem.h"

AssimpMemoryStream::AssimpMemoryStream(std::vector<uint8_t> contents) : contents(std::move(contents)) {}

size_t AssimpMemoryStream::Read(void *buffer, size_t size, size_t count) {
    if (size == 0 || count == 0) {
        return 0;
    }
    size_t elementCount = std::min(count, (contents.size() - position) / size);
    std::memcpy(buffer, contents.data() + position, elementCount * size);
    position += elementCount * size;
    return elementCount;
}

size_t AssimpMemoryStream::Write(const void *buffer [[gnu::unused]], size_t size [[gnu::unused]], size_t count [[gnu::unused]]) {
    return 0;
}

aiReturn AssimpMemoryStream::Seek(size_t offset, aiOrigin origin) {
    size_t newPosition;
    switch (origin) {
        case aiOrigin_SET:
            newPosition = offset;
            break;
        case aiOrigin_CUR:
            newPosition = position + offset;//importers pass backward seeks as a wrapped size_t, unsigned addition still lands right
            break;
        case aiOrigin_END:
            if (offset > contents.size()) {
                return aiReturn_FAILURE;
            }
            newPosition = contents.size() - offset;
            break;
        default:
            return aiReturn_FAILURE;
    }
    if (newPosition > contents.size()) {
        return aiReturn_FAILURE;
    }
    position = newPosition;
    return aiReturn_SUCCESS;
}

size_t AssimpMemoryStream::Tell() const {
    return position;
}

size_t AssimpMemoryStream::FileSize() const {
    return contents.size();
}

void AssimpMemoryStream::Flush() {
}

bool AssimpFileSystemIO::Exists(const char *file) const {
    return LimonFileSystem::fileExists(file);
}

char AssimpFileSystemIO::getOsSeparator() const {
    return '/';
}

Assimp::IOStream *AssimpFileSystemIO::Open(const char *file, const char *mode) {
    if (std::strchr(mode, 'w') != nullptr || std::strchr(mode, 'a') != nullptr) {
        std::cerr << "Assimp asked to write " << file << ", model import only reads." << std::endl;
        return nullptr;
    }
    std::vector<uint8_t> contents;
    if (!LimonFileSystem::readFile(file, contents)) {
        return nullptr;
    }
    return new AssimpMemoryStream(std::move(contents));
}

void AssimpFileSystemIO::Close(Assimp::IOStream *stream) {
    delete stream;
}
