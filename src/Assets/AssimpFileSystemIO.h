//
// Created by engin on 14.10.2026.
//

#ifndef LIMONENGINE_ASSIMPFILESYSTEMIO_H
#define LIMONENGINE_ASSIMPFILESYSTEMIO_H

#include <cstdint>
#include <vector>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

class AssimpMemoryStream : public Assimp::IOStream {
    std::vector<uint8_t> contents;
    size_t position = 0;
public:
    explicit AssimpMemoryStream(std::vector<uint8_t> contents);

    size_t Read(void *buffer, size_t size, size_t count) override;

    size_t Write(const void *buffer, size_t size, size_t count) override;

    aiReturn Seek(size_t offset, aiOrigin origin) override;

    size_t Tell() const override;

    size_t FileSize() const override;

    void Flush() override;
};

//importers open sibling files themselves (mtl, external animations), handing assimp a memory buffer would miss those
class AssimpFileSystemIO : public Assimp::IOSystem {
public:
    bool Exists(const char *file) const override;

    char getOsSeparator() const override;

    Assimp::IOStream *Open(const char *file, const char *mode) override;

    void Close(Assimp::IOStream *stream) override;
};


#endif //LIMONENGINE_ASSIMPFILESYSTEMIO_H
