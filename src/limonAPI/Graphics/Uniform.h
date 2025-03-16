//
// Created by engin on 11.10.2020.
//

#ifndef LIMONENGINE_UNIFORM_H
#define LIMONENGINE_UNIFORM_H


#include <string>

class Uniform{
public:
    enum class VariableTypes {
        BOOL,
        INT,
        FLOAT,
        FLOAT_VEC2,
        FLOAT_VEC3,
        FLOAT_VEC4,
        FLOAT_MAT4,
        CUBEMAP,
        CUBEMAP_ARRAY,
        TEXTURE_2D,
        TEXTURE_2D_ARRAY,
        UNDEFINED
    };

    unsigned int location;
    std::string name;
    VariableTypes type;
    unsigned int size;

    Uniform(unsigned int location, const std::string &name, VariableTypes type, unsigned int size) : location(
            location), name(name), type(type), size(size) {}
};


#endif //LIMONENGINE_UNIFORM_H
