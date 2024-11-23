//
// Created by engin on 5.11.2017.
//

#ifndef LIMONENGINE_OPTIONS_H
#define LIMONENGINE_OPTIONS_H

#include <glm/glm.hpp>
#include <iostream>
#include <tinyxml2.h>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstring>
#include "Utils/HashUtil.h"
#include "Utils/Logger.h"
#include "API/LimonTypes.h"

namespace OptionsUtil {
    class Options {
    public:
        static constexpr float PI = 3.14159265358979f;
        static constexpr float PI_DOUBLE = 3.141592653589793238463;
        enum class MoveModes {
            WALK, RUN
        };
        enum class TextureFilteringModes {
            NEAREST, BILINEAR, TRILINEAR
        };

        template<typename T>
        class Option {
            std::shared_ptr<LimonTypes::GenericParameter> value = nullptr;
            bool isSet = false;
            std::vector<long>* longValues = nullptr;
            explicit Option(const std::shared_ptr<LimonTypes::GenericParameter> &value, bool isSet) : value(value), isSet(isSet) {
                if(value->valueType == LimonTypes::GenericParameter::LONG_ARRAY) {
                    longValues = new std::vector<long>();
                    longValues->reserve(value->value.longValues[0]);
                }
            }
            friend class Options;

        public:
            explicit Option() = default;
            bool isUsable() const { return isSet; };

            template<typename Q =  T, typename std::enable_if<!std::is_same<Q, std::string>::value>::type* = nullptr, typename std::enable_if<!std::is_same<Q, std::vector<long>>::value>::type* = nullptr>
            T get() const {
                if (!isSet) {
                    std::cerr << "Option " << value->description << " is not set" << std::endl;
                }
                return *((T *) &(value->value));
            };

            template<typename Q =  T, typename std::enable_if<!std::is_same<Q, std::string>::value>::type* = nullptr, typename std::enable_if<!std::is_same<Q, std::vector<long>>::value>::type* = nullptr>
            T getOrDefault(T defaultValue) const {
                if (!isSet) {
                    return defaultValue;
                }
                return *((T *) &(value->value));
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, std::string>::value>::type* = nullptr>
            std::string get() const {
                if (!isSet) {
                    std::cerr << "Option " << value->description << " is not set" << std::endl;
                }
                return value->value.stringValue;
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, std::string>::value>::type* = nullptr>
            std::string getOrDefault(T defaultValue) const {
                if (!isSet) {
                    return defaultValue;
                }

                return value->value.stringValue;
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, std::vector<long>>::value>::type* = nullptr>
            std::vector<long> get() const {
                if (!isSet) {
                    std::cerr << "Option " << value->description << " is not set" << std::endl;
                }
                for(long i=1; i < value->value.longValues[0];++i) {
                    longValues->emplace_back(value->value.longValues[i]);
                }
                return *longValues;
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, std::vector<long>>::value>::type* = nullptr>
            std::vector<long> getOrDefault(T defaultValue) const {
                if (!isSet) {
                    return defaultValue;
                }
                for(long i=1; i < value->value.longValues[0];++i) {
                    longValues->emplace_back(value->value.longValues[i]);
                }
                return *longValues;
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, std::vector<long>>::value>::type* = nullptr>
            bool set(std::vector<long> newValue) {
                if(newValue.size() > 15) {
                    return false;
                }
                value->value.longValues[0] = newValue.size();
                for(size_t i=1; i < newValue.size(); ++i){
                    value->value.longValues[i] = newValue[i-1];
                }
                isSet = true;
                return true;
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, std::string>::value>::type* = nullptr>
            bool set(const std::string& newValue) {
                if(newValue.length() > 63) {
                    return false;
                }
                memset(&(value->value), 0, sizeof(value));
                strncpy(value->value.stringValue, newValue.c_str(), newValue.length());
                isSet = true;
                return true;
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, LimonTypes::Vec4>::value>::type* = nullptr>
            bool set(const LimonTypes::Vec4& newValue)  {
                value->value.vectorValue = newValue;
                this->isSet = true;
                return true;
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, LimonTypes::Mat4>::value>::type* = nullptr>
            bool set(const LimonTypes::Mat4& newValue)  {
                value->value.matrixValue = newValue;
                this->isSet = true;
                return true;
            };

            template<typename Q =  T, typename std::enable_if<std::is_same<Q, double>::value>::type* = nullptr>
            bool set(double newValue)  {
                value->value.doubleValue = newValue;
                this->isSet = true;
                return true;
            };


        };

        template<class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
        Option<std::string> getOption(uint64_t optionHash) const {
            auto it = this->options.find(optionHash);
            if (it != this->options.end() && it->second->valueType == LimonTypes::GenericParameter::STRING) {
                return Option<std::string>(it->second, true);
            }
            return Option<std::string>(nullptr, false);
        }

        template<class T, typename std::enable_if<std::is_same<T, double>::value>::type* = nullptr>
        Option<double> getOption(uint64_t optionHash) const {
            auto it = this->options.find(optionHash);
            if (it != this->options.end() && it->second->valueType == LimonTypes::GenericParameter::DOUBLE) {
                return Option<double>(it->second, true);
            }
            return Option<double>(nullptr, false);
        }

        template<class T, typename std::enable_if<std::is_same<T, long>::value>::type* = nullptr>
        Option<long> getOption(uint64_t optionHash) const {
            auto it = this->options.find(optionHash);
            if (it != this->options.end() && it->second->valueType == LimonTypes::GenericParameter::LONG) {
                return Option<long>(it->second, true);
            }
            return Option<long>(nullptr, false);
        }

        template<class T, typename std::enable_if<std::is_same<T, std::vector<long>>::value>::type* = nullptr>
        Option<std::vector<long>> getOption(uint64_t optionHash) const {
            auto it = this->options.find(optionHash);
            if (it != this->options.end() && it->second->valueType == LimonTypes::GenericParameter::LONG_ARRAY) {
                return Option<std::vector<long>>(it->second, true);
            }
            return Option<std::vector<long>>(nullptr, false);
        }

        template<class T, typename std::enable_if<std::is_same<T, bool>::value>::type* = nullptr>
        Option<bool> getOption(uint64_t optionHash) const {
            auto it = this->options.find(optionHash);
            if (it != this->options.end() && it->second->valueType == LimonTypes::GenericParameter::BOOLEAN) {
                return Option<bool>(it->second, true);
            }
            return Option<bool>(nullptr, false);
        }

        template<class T, typename std::enable_if<std::is_same<T, LimonTypes::Vec4>::value>::type* = nullptr>
        Option<LimonTypes::Vec4> getOption(uint64_t optionHash) const {
            auto it = this->options.find(optionHash);
            if (it != this->options.end() && it->second->valueType == LimonTypes::GenericParameter::VEC4) {
                return Option<LimonTypes::Vec4>(it->second, true);
            }
            return Option<LimonTypes::Vec4>(nullptr, false);
        }

        template<class T, typename std::enable_if<std::is_same<T, LimonTypes::Mat4>::value>::type* = nullptr>
        Option<LimonTypes::Mat4> getOption(uint64_t optionHash) const {
            auto it = this->options.find(optionHash);
            if (it != this->options.end() && it->second->valueType == LimonTypes::GenericParameter::MAT4) {
                return Option<LimonTypes::Mat4>(it->second, true);
            }
            return Option<LimonTypes::Mat4>(nullptr, false);
        }

        /**
         * This method is a workaround for API usage, it will find the hash of the string by
         * iterating over all the options. If no option with that name exists, it will return 0.
         *
         * If it returns zero it means option is not set
         *
         * @param optionName
         * @return hash of that Option or 0 if not found
         */
        uint64_t getHash(const std::string &optionName) const {
            for(auto option:this->options) {
                if(option.second->description == optionName) {
                    return option.first;
                }
            }
            return 0;
        }

    private:
        Logger *logger{};
        std::unordered_map<uint64_t, std::shared_ptr<LimonTypes::GenericParameter>> options;

        Option<long> heightOption;
        Option<long> widthOption;

        /*SDL properties that should be available */
        void *imeWindowHandle;
        int drawableWidth, drawableHeight;
        int windowWidth, windowHeight;
        bool isWindowInFocus;

        bool loadVec3(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec3 &);

        bool loadVec4(tinyxml2::XMLNode *optionsNode, const std::string &name, glm::vec4 &);

        bool loadDouble(tinyxml2::XMLNode *optionsNode, const std::string &name, double &);

        bool loadLong(tinyxml2::XMLNode *optionsNode, const std::string &name, long &);

        bool loadString(tinyxml2::XMLNode *optionsNode, const std::string &name, std::string &);

        bool loadBool(tinyxml2::XMLNode *optionsNode, const std::string &name, bool &);

    public:

        bool loadOptionsNew(const std::string &optionsFileName);

        void *getImeWindowHandle() const {
            return imeWindowHandle;
        }

        void setImeWindowHandle(void *imeWindowHandle) {
            Options::imeWindowHandle = imeWindowHandle;
        }

        int getDrawableWidth() const {
            return drawableWidth;
        }

        void setDrawableWidth(int drawableWidth) {
            Options::drawableWidth = drawableWidth;
        }

        int getDrawableHeight() const {
            return drawableHeight;
        }

        void setDrawableHeight(int drawableHeight) {
            Options::drawableHeight = drawableHeight;
        }

        int getWindowWidth() const {
            return windowWidth;
        }

        void setWindowWidth(int windowWidth) {
            Options::windowWidth = windowWidth;
        }

        int getWindowHeight() const {
            return windowHeight;
        }

        void setWindowHeight(int windowHeight) {
            Options::windowHeight = windowHeight;
        }

        bool isIsWindowInFocus() const {
            return isWindowInFocus;
        }

        void setIsWindowInFocus(bool isWindowInFocus) {
            Options::isWindowInFocus = isWindowInFocus;
        }

        uint32_t getScreenHeight() const {
            return heightOption.getOrDefault(1080);
        }

        uint32_t getScreenWidth() const {
            return widthOption.getOrDefault(1920);
        }

        Options() {
            this->logger = new Logger();
        };

        Logger *getLogger() {
            return logger;
        }

        /**
         * TODO: This method is doing a copy, because our API can't depend on cityHash. There might be alternatives, needs investigation
         *
         * @return all options in a copy
         */
        const std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>> getAllOptions() const {
            std::unordered_map<std::string, std::shared_ptr<LimonTypes::GenericParameter>> optionsToReturn;
            for (auto option:options) {
                optionsToReturn[option.second->description] = option.second;
            }
            return optionsToReturn;
        }

    };

}
#endif //LIMONENGINE_OPTIONS_H
