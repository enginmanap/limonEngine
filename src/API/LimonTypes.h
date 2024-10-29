//
// Created by engin on 25.07.2021.
//

#ifndef LIMONENGINE_LIMONTYPES_H
#define LIMONENGINE_LIMONTYPES_H

#include <cassert>
#include <string>

namespace LimonTypes {

    struct Vec2 {
        float x, y;

        Vec2() = default;

        Vec2(float x, float y) : x(x), y(y) {}

        float operator[](int i) const {
            assert((i == 0 || i == 1) && "Access to undefined element of vector");
            switch (i) {
                default:
                case 0:
                    return x;
                case 1:
                    return y;
            }
        }

        float &operator[](int i) {
            assert((i == 0 || i == 1) && "Access to undefined element of vector");
            switch (i) {
                default:
                case 0:
                    return x;
                case 1:
                    return y;
            }
        }
    };


    struct Vec4 {
        float x, y, z, w;

        Vec4() = default;

        Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

        Vec4(float x, float y, float z) : x(x), y(y), z(z), w(0) {}

        float operator[](int i) const {
            assert((i >= 0 && i < 4) && "Access to undefined element of vector");
            switch (i) {
                default:
                case 0:
                    return x;
                case 1:
                    return y;
                case 2:
                    return z;
                case 3:
                    return w;
            }
        }

        float &operator[](int i) {
            assert((i >= 0 && i < 4) && "Access to undefined element of vector");
            switch (i) {
                default:
                case 0:
                    return x;
                case 1:
                    return y;
                case 2:
                    return z;
                case 3:
                    return w;
            }
        }

        Vec4 operator+(const Vec4 &second) const {
            Vec4 result{};
            result.x = this->x + second.x;
            result.y = this->y + second.y;
            result.z = this->z + second.z;
            result.w = this->w + second.w;
            return result;
        }

        Vec4 operator-(const Vec4 &second) const {
            Vec4 result{};
            result.x = this->x - second.x;
            result.y = this->y - second.y;
            result.z = this->z - second.z;
            result.w = this->w - second.w;
            return result;
        }

        Vec4 operator*(float scalar) const {
            Vec4 result{};
            result.x = this->x * scalar;
            result.y = this->y * scalar;
            result.z = this->z * scalar;
            result.w = this->w * scalar;
            return result;
        }
    };

    struct Mat4 {
        Vec4 rows[4];

        Mat4() = default;

        Mat4(Vec4 row0, Vec4 row1, Vec4 row2, Vec4 row3) : rows() {//rows init is just to make static analyzer happy
            rows[0] = row0;
            rows[1] = row1;
            rows[2] = row2;
            rows[3] = row3;
        }

        Vec4 operator[](int i) const { return rows[i]; }

        Vec4 &operator[](int i) { return rows[i]; }
    };

    struct GenericParameter {
        enum RequestParameterTypes {
            MODEL, ANIMATION, SWITCH, FREE_TEXT, TRIGGER, GUI_TEXT, FREE_NUMBER, COORDINATE, TRANSFORM, MULTI_SELECT
        };
        RequestParameterTypes requestType = FREE_NUMBER;
        std::string description;
        enum ValueTypes {
            STRING, DOUBLE, LONG, LONG_ARRAY, BOOLEAN, VEC4, MAT4
        };
        ValueTypes valueType = ValueTypes::VEC4;
        //Up part used for requesting parameter, down part used as values of that request.
        union Value {
            char stringValue[64];
            long longValue;
            long longValues[16];//first element is the size
            double doubleValue;
            Vec4 vectorValue;
            Mat4 matrixValue;
            bool boolValue;
        };

        Value value{};
        bool isSet = false;

        GenericParameter() {
            memset(&value, 0, sizeof(value));
        };

        std::string to_string() {
            switch (valueType) {
                case STRING:
                    return value.stringValue;
                case DOUBLE:
                    return std::to_string(value.doubleValue);
                case LONG:
                    return std::to_string(value.longValue);
                case LONG_ARRAY: {
                        long size = value.longValues[0];
                        std::string valueString = "[";
                        for (long i = 1; i < size - 1; ++i) {
                            valueString += std::to_string(value.longValues[i]) + ", ";
                        }
                        valueString += std::to_string(value.longValues[size-1]) + "]";
                        return valueString;
                    }
                    break;
                case BOOLEAN:
                    return std::to_string(value.boolValue);
                case VEC4: {
                        std::string valueString = "[";
                        for (size_t i = 0; i < 3; ++i) {
                            valueString += std::to_string(value.vectorValue[i]) + ", ";
                        }
                        valueString += std::to_string(value.vectorValue[3]) + "]";
                        return valueString;
                    }
                case MAT4:
                {
                    std::string valueString = "[";
                    for (size_t i = 0; i < 4; ++i) {
                        for (size_t j = 0; j < 4; ++j) {
                            valueString += std::to_string(value.matrixValue.rows[i][j]) + ", ";
                        }
                    }
                    valueString.replace(valueString.length()-1, 1, "]");//remove the last
                    return valueString;
                }
            }
            std::cerr << "Type info not matched any case, this is an error" << std::endl;
            return "";
        }
    };

}

#endif //LIMONENGINE_LIMONTYPES_H