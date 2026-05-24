//
// Header-only pybind11 type casters for GLM types.
// Must be included in every translation unit that performs pybind11 casts involving
// glm::vec2, glm::vec3, glm::quat, std::vector<glm::vec3>, or std::vector<unsigned int>.
//
#ifndef LIMONENGINE_PYTHONTYPECASTERS_H
#define LIMONENGINE_PYTHONTYPECASTERS_H
#include <pybind11/pybind11.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

// Conversion for Vec2 (used by GUI position/scale parameters)
template<>
class pybind11::detail::type_caster<glm::vec2> {
public:
    PYBIND11_TYPE_CASTER(glm::vec2, _("glm::vec2"));

    bool load(handle src, bool) {
        if (!pybind11::isinstance<pybind11::object>(src))
            return false;

        pybind11::object obj = pybind11::reinterpret_borrow<pybind11::object>(src);
        if (pybind11::isinstance<pybind11::sequence>(obj)) {
            pybind11::sequence seq = obj.cast<pybind11::sequence>();
            if (seq.size() < 2) return false;
            value.x = seq[0].cast<float>();
            value.y = seq[1].cast<float>();
            return true;
        }
        if (pybind11::hasattr(obj, "x") && pybind11::hasattr(obj, "y")) {
            value.x = obj.attr("x").cast<float>();
            value.y = obj.attr("y").cast<float>();
            return true;
        }
        return false;
    }

    static handle cast(const glm::vec2 &src, return_value_policy, handle) {
        pybind11::dict d;
        d["x"] = src.x;
        d["y"] = src.y;
        return d.release();
    }
};

// Conversion for Vec3
template<>
class pybind11::detail::type_caster<glm::vec3> {
public:
    PYBIND11_TYPE_CASTER(glm::vec3, _("glm::vec3"));

    bool load(handle src, bool) {
        if (!pybind11::isinstance<pybind11::object>(src))
            return false;

        pybind11::object obj = pybind11::reinterpret_borrow<pybind11::object>(src);
        if (!pybind11::hasattr(obj, "x") || !pybind11::hasattr(obj, "y") || !pybind11::hasattr(obj, "z"))
            return false;

        value.x = obj.attr("x").cast<float>();
        value.y = obj.attr("y").cast<float>();
        value.z = obj.attr("z").cast<float>();
        return true;
    }

    static handle cast(const glm::vec3 &src, return_value_policy, handle) {
        // Always use dictionaries during initialization to avoid import order issues
        // The C++ side will handle conversion to Vec3 objects at runtime
        pybind11::dict d;
        d["x"] = src.x;
        d["y"] = src.y;
        d["z"] = src.z;
        return d.release();
    }
};

// Conversion for quaternion
template<>
class pybind11::detail::type_caster<glm::quat> {
public:
    PYBIND11_TYPE_CASTER(glm::quat, _("glm::quat"));

    bool load(handle src, bool) {
        if (!pybind11::isinstance<pybind11::object>(src))
            return false;

        pybind11::object obj = pybind11::reinterpret_borrow<pybind11::object>(src);
        if (!pybind11::hasattr(obj, "x") || !pybind11::hasattr(obj, "y") ||
            !pybind11::hasattr(obj, "z") || !pybind11::hasattr(obj, "w"))
            return false;

        float x = obj.attr("x").cast<float>();
        float y = obj.attr("y").cast<float>();
        float z = obj.attr("z").cast<float>();
        float w = obj.attr("w").cast<float>();

        value = glm::quat(w, x, y, z);
        return true;
    }

    static handle cast(const glm::quat &src, return_value_policy, handle) {
        pybind11::dict d;
        d["x"] = src.x;
        d["y"] = src.y;
        d["z"] = src.z;
        d["w"] = src.w;
        return d.release();
    }
};

template<>
class pybind11::detail::type_caster<std::vector<glm::vec3>> {
public:
    PYBIND11_TYPE_CASTER(std::vector<glm::vec3>, _("List[glm::vec3]"));

    bool load(handle src, bool) {
        if (!pybind11::isinstance<pybind11::sequence>(src))
            return false;

        pybind11::sequence seq = pybind11::reinterpret_borrow<pybind11::sequence>(src);
        for (pybind11::handle item : seq) {
            glm::vec3 vec = pybind11::cast<glm::vec3>(item);
            value.push_back(vec);
        }
        return !seq.empty();
    }

    static handle cast(const std::vector<glm::vec3>& src, return_value_policy, handle) {
        pybind11::list list;
        for (const glm::vec3& vec : src) {
            list.append(pybind11::cast(vec));
        }
        return list.release();
    }
};

template<>
class pybind11::detail::type_caster<std::vector<unsigned int>> {
public:
    PYBIND11_TYPE_CASTER(std::vector<unsigned int>, _("List[int]"));

    bool load(handle src, bool) {
        if (!pybind11::isinstance<pybind11::sequence>(src))
            return false;

        pybind11::sequence seq = pybind11::reinterpret_borrow<pybind11::sequence>(src);
        value.clear();
        value.reserve(seq.size());

        for (const pybind11::handle &item : seq) {
            value.push_back(item.cast<unsigned int>());
        }
        return true;
    }

    static handle cast(const std::vector<unsigned int>& src, return_value_policy, handle) {
        pybind11::list list;
        for (const unsigned int& item : src) {
            list.append(item);
        }
        return list.release();
    }
};

#endif //LIMONENGINE_PYTHONTYPECASTERS_H
