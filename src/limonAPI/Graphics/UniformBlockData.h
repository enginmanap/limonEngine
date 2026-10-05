#ifndef LIMONENGINE_UNIFORMBLOCKDATA_H
#define LIMONENGINE_UNIFORMBLOCKDATA_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//Members of a uniform block in declaration order, as plain values. The backend lays them out, so no padding here.
//Arrays of int/float/vec2/vec3 can't be added as repeated members, their elements are strided to 16 bytes in the block.
class UniformBlockData {
public:
    enum class MemberTypes {INT, FLOAT, VEC2, VEC3, VEC4, MAT3, MAT4, STRUCT_BEGIN, STRUCT_END};

    struct Member {
        MemberTypes type;
        uint32_t valueOffset;//into values, tightly packed
    };

private:
    std::vector<Member> members;
    std::vector<uint8_t> values;

    void addMember(MemberTypes type, const void* value, uint32_t valueSize) {
        members.push_back(Member{type, static_cast<uint32_t>(values.size())});
        const uint8_t* valueBytes = static_cast<const uint8_t*>(value);
        values.insert(values.end(), valueBytes, valueBytes + valueSize);
    }

public:
    //keeps the capacity, so refilling every frame doesn't allocate
    void clear() {
        members.clear();
        values.clear();
    }

    void addInt(int32_t value) { addMember(MemberTypes::INT, &value, sizeof(value)); }
    void addFloat(float value) { addMember(MemberTypes::FLOAT, &value, sizeof(value)); }
    void addVec2(const glm::vec2& value) { addMember(MemberTypes::VEC2, glm::value_ptr(value), sizeof(value)); }
    void addVec3(const glm::vec3& value) { addMember(MemberTypes::VEC3, glm::value_ptr(value), sizeof(value)); }
    void addVec4(const glm::vec4& value) { addMember(MemberTypes::VEC4, glm::value_ptr(value), sizeof(value)); }
    void addMat3(const glm::mat3& value) { addMember(MemberTypes::MAT3, glm::value_ptr(value), sizeof(value)); }
    void addMat4(const glm::mat4& value) { addMember(MemberTypes::MAT4, glm::value_ptr(value), sizeof(value)); }
    void beginStruct() { addMember(MemberTypes::STRUCT_BEGIN, nullptr, 0); }
    void endStruct() { addMember(MemberTypes::STRUCT_END, nullptr, 0); }

    const std::vector<Member>& getMembers() const { return members; }
    const uint8_t* getValue(const Member& member) const { return values.data() + member.valueOffset; }
};

#endif //LIMONENGINE_UNIFORMBLOCKDATA_H
