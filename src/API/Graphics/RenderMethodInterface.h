//
// Created by engin on 23.09.2019.
//

#ifndef LIMONENGINE_RENDERMETHODINTERFACE_H
#define LIMONENGINE_RENDERMETHODINTERFACE_H

#include <map>
#include <string>
#include <vector>

#include "API/LimonAPI.h"
#include "API/Graphics/GraphicsProgram.h"

class GraphicsInterface;

class RenderMethodInterface {
/**
 * On dynamic library load, void registerDynamicRenderMethod(std::map<std::string, RenderMethodInterface *(*constructor)(GraphicsInterface *)) function should be callable.
 *
 * This method will be called after object load.
 * For each render method, the name of trigger and a constructor should be put in the map.
 */
    static std::map<std::string, RenderMethodInterface *(*)(GraphicsInterface *)> *RenderMethodsMap;
protected:
    GraphicsInterface *graphicsInterface = nullptr;
    static std::map<std::string, RenderMethodInterface * (*)(GraphicsInterface * )> *getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!RenderMethodsMap) {
            RenderMethodsMap = new std::map<std::string, RenderMethodInterface * (*)(GraphicsInterface * )>();
        }
        return RenderMethodsMap;
    }

    explicit RenderMethodInterface(GraphicsInterface
    * graphicsInterface):
            graphicsInterface(graphicsInterface) {}

    //proxy the texture methods
    uint32_t createTexture(int height, int width, GraphicsInterface::TextureTypes type,
                           GraphicsInterface::InternalFormatTypes internalFormat,
                           GraphicsInterface::FormatTypes format,
                           GraphicsInterface::DataTypes dataType,
                           uint32_t depth) {
        return graphicsInterface->createTexture(height, width, type, internalFormat, format, dataType, depth);
    }

    bool deleteTexture(uint32_t textureID) {
        return graphicsInterface->deleteTexture(textureID);
    }

    void setWrapMode(uint32_t textureID, GraphicsInterface::TextureTypes textureType, GraphicsInterface::TextureWrapModes wrapModeS,
                             GraphicsInterface::TextureWrapModes wrapModeT, GraphicsInterface::TextureWrapModes wrapModeR) {
        graphicsInterface->setWrapMode(textureID, textureType, wrapModeS, wrapModeT, wrapModeR);
    }

    void setTextureBorder(uint32_t textureID, GraphicsInterface::TextureTypes textureType, bool isBorderColorSet,
                                  const std::vector<float> &borderColors) {
        graphicsInterface->setTextureBorder(textureID, textureType, isBorderColorSet, borderColors);
    }

    void setFilterMode(uint32_t textureID, GraphicsInterface::TextureTypes textureType, GraphicsInterface::FilterModes filterMode) {
        graphicsInterface->setFilterMode(textureID, textureType, filterMode);
    }

    void loadTextureData(uint32_t textureID, int height, int width, GraphicsInterface::TextureTypes type, GraphicsInterface::InternalFormatTypes internalFormat, GraphicsInterface::FormatTypes format, GraphicsInterface::DataTypes dataType, uint32_t depth,
                                 void *data, void *data2, void *data3, void *data4, void *data5, void *data6) {
        graphicsInterface->loadTextureData(textureID, height, width, type, internalFormat, format, dataType, depth, data, data2, data3, data4, data5, data6);
    }

public:
    // Not virtual
    static std::vector<std::string> getRenderMethodNames() {
        std::vector<std::string> names;
        for (auto & it : *RenderMethodsMap) {
            names.push_back(it.first);
        }
        return names;
    }

    static void registerDynamicRenderMethod(const std::string &typeName, RenderMethodInterface *(*constructor)(GraphicsInterface *)) {
        (*getMap())[typeName] = constructor;
    }

    virtual std::vector<LimonTypes::GenericParameter> getParameters() const = 0;

    virtual bool initRender(std::shared_ptr<GraphicsProgram> program[[gnu::unused]],
                            std::vector<LimonTypes::GenericParameter> parameters[[gnu::unused]]) { return true;};

    virtual void renderFrame(std::shared_ptr<GraphicsProgram> program[[gnu::unused]]) {};

    virtual bool cleanupRender(std::shared_ptr<GraphicsProgram> program[[gnu::unused]],
                               std::vector<LimonTypes::GenericParameter> parameters[[gnu::unused]]) { return true;};

    virtual ~RenderMethodInterface() = default;

    virtual std::string getName() const = 0;

    static RenderMethodInterface *createRenderMethodInterfaceInstance(std::string const &methodName, GraphicsInterface *graphicsInterface) {
        auto it = getMap()->find(methodName);
        if (it == getMap()->end()) {
            return nullptr;
        }
        return it->second(graphicsInterface);
    }

};

template<typename T>
RenderMethodInterface *createT(GraphicsInterface *graphicsInterface) {
    return new T(graphicsInterface);
}

template<typename T>
class RenderMethodRegister : RenderMethodInterface {
    virtual std::vector<LimonTypes::GenericParameter> getParameters() const override {
        return std::vector<LimonTypes::GenericParameter>();
    };

    virtual bool initRender(std::shared_ptr<GraphicsProgram> program [[gnu::unused]], std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {return false;};

    virtual void renderFrame(std::shared_ptr<GraphicsProgram> program [[gnu::unused]], std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {};

    virtual bool cleanupRender(std::shared_ptr<GraphicsProgram> program [[gnu::unused]], std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {return false;};

    std::string getName() const override {
        return "This object is not meant to be used";
    }


public:
    RenderMethodRegister(std::string const &s) : RenderMethodInterface(nullptr) {
        getMap()->insert(std::make_pair(s, &createT<T>));
    }
};


#endif //LIMONENGINE_RENDERMETHODINTERFACE_H
