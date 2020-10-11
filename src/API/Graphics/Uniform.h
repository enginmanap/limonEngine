//
// Created by engin on 11.10.2020.
//

#ifndef LIMONENGINE_UNIFORM_H
#define LIMONENGINE_UNIFORM_H


#include <string>
#include <GL/glew.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else

#include <GL/gl.h>
#endif/*__APPLE__*/



class Uniform{
public:
    enum class VariableTypes {
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

    Uniform(unsigned int location, const std::string &name, GLenum typeEnum, unsigned int size) : location(
            location), name(name), size(size) {
        switch (typeEnum) {
            case GL_SAMPLER_CUBE:
                type = VariableTypes::CUBEMAP;
                break;
            case GL_SAMPLER_CUBE_MAP_ARRAY_ARB:
                type = VariableTypes::CUBEMAP_ARRAY;
                break;
            case GL_SAMPLER_2D:
                type = VariableTypes::TEXTURE_2D;
                break;
            case GL_SAMPLER_2D_ARRAY:
                type = VariableTypes::TEXTURE_2D_ARRAY;
                break;
            case GL_INT:
                type = VariableTypes::INT;
                break;
            case GL_FLOAT:
                type = VariableTypes::FLOAT;
                break;
            case GL_FLOAT_VEC2:
                type = VariableTypes::FLOAT_VEC2;
                break;
            case GL_FLOAT_VEC3:
                type = VariableTypes::FLOAT_VEC3;
                break;
            case GL_FLOAT_VEC4:
                type = VariableTypes::FLOAT_VEC4;
                break;
            case GL_FLOAT_MAT4:
                type = VariableTypes::FLOAT_MAT4;
                break;
            default:
                type = VariableTypes::UNDEFINED;
        }
    }
};


#endif //LIMONENGINE_UNIFORM_H
