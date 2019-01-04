//
// Created by engin on 3.01.2019.
//

#ifndef LIMONENGINE_GLMCEREALCONVERTERS_HPP
#define LIMONENGINE_GLMCEREALCONVERTERS_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cereal/cereal.hpp>

namespace glm {

template<typename Archive> void serialize(Archive& archive, glm::vec2& v2) {
    archive(cereal::make_nvp("x", v2.x), cereal::make_nvp("y", v2.y));
}

template<typename Archive> void serialize(Archive& archive, glm::vec3& v3) {
    archive(cereal::make_nvp("x", v3.x), cereal::make_nvp("y", v3.y), cereal::make_nvp("z", v3.z));
}

template<typename Archive> void serialize(Archive& archive, glm::vec4& v4) {
    archive(cereal::make_nvp("x", v4.x), cereal::make_nvp("y", v4.y), cereal::make_nvp("z", v4.z), cereal::make_nvp("w", v4.w));
}

template<typename Archive> void serialize(Archive& archive, glm::mediump_uvec3& v3) {
    archive(cereal::make_nvp("x", v3.x), cereal::make_nvp("y", v3.y), cereal::make_nvp("z", v3.z));
}

template<typename Archive> void serialize(Archive& archive, glm::lowp_uvec4& v4) {
    archive(cereal::make_nvp("x", v4.x), cereal::make_nvp("y", v4.y), cereal::make_nvp("z", v4.z), cereal::make_nvp("w", v4.w));
}

template<typename Archive> void serialize(Archive& archive, glm::quat& quat) {
    archive(cereal::make_nvp("x", quat.x), cereal::make_nvp("y", quat.y), cereal::make_nvp("z", quat.z), cereal::make_nvp("w", quat.w));
}

template<typename Archive> void serialize(Archive& archive, glm::mat4& m4) {
    archive(
        cereal::make_nvp("00", m4[0][0]), cereal::make_nvp("01", m4[0][1]), cereal::make_nvp("02", m4[0][2]), cereal::make_nvp("03", m4[0][3]),
        cereal::make_nvp("10", m4[1][0]), cereal::make_nvp("11", m4[1][1]), cereal::make_nvp("12", m4[1][2]), cereal::make_nvp("13", m4[1][3]),
        cereal::make_nvp("20", m4[2][0]), cereal::make_nvp("21", m4[2][1]), cereal::make_nvp("22", m4[2][2]), cereal::make_nvp("23", m4[2][3]),
        cereal::make_nvp("30", m4[3][0]), cereal::make_nvp("31", m4[3][1]), cereal::make_nvp("32", m4[3][2]), cereal::make_nvp("33", m4[3][3])
    );
}

}


#endif //LIMONENGINE_GLMCEREALCONVERTERS_HPP
