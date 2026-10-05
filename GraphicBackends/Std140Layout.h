#ifndef LIMONENGINE_STD140LAYOUT_H
#define LIMONENGINE_STD140LAYOUT_H

#include <cstdint>
#include <vector>
#include "limonAPI/Graphics/UniformBlockData.h"

//std140 rules for the members UniformBlockData can hold, shared by both backends
class Std140Layout {
    static void alignTo(std::vector<uint8_t> &blockBytes, size_t alignment) {
        blockBytes.resize((blockBytes.size() + alignment - 1) / alignment * alignment, 0);
    }

    static void append(std::vector<uint8_t> &blockBytes, const uint8_t *value, size_t size, size_t alignment) {
        alignTo(blockBytes, alignment);
        blockBytes.insert(blockBytes.end(), value, value + size);
    }

public:
    static void layout(const UniformBlockData &data, std::vector<uint8_t> &blockBytes) {
        blockBytes.clear();
        for (const UniformBlockData::Member &member : data.getMembers()) {
            const uint8_t* value = data.getValue(member);
            switch (member.type) {
                case UniformBlockData::MemberTypes::INT:
                case UniformBlockData::MemberTypes::FLOAT:
                    append(blockBytes, value, 4, 4);
                    break;
                case UniformBlockData::MemberTypes::VEC2:
                    append(blockBytes, value, 8, 8);
                    break;
                case UniformBlockData::MemberTypes::VEC3:
                    append(blockBytes, value, 12, 16);//a scalar after it fills the 4th component
                    break;
                case UniformBlockData::MemberTypes::VEC4:
                    append(blockBytes, value, 16, 16);
                    break;
                case UniformBlockData::MemberTypes::MAT3:
                    for (size_t column = 0; column < 3; ++column) {
                        append(blockBytes, value + column * 12, 12, 16);
                    }
                    alignTo(blockBytes, 16);
                    break;
                case UniformBlockData::MemberTypes::MAT4:
                    append(blockBytes, value, 64, 16);
                    break;
                case UniformBlockData::MemberTypes::STRUCT_BEGIN:
                case UniformBlockData::MemberTypes::STRUCT_END:
                    alignTo(blockBytes, 16);
                    break;
            }
        }
        alignTo(blockBytes, 16);
    }
};

#endif //LIMONENGINE_STD140LAYOUT_H
