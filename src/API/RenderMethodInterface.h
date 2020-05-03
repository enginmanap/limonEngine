//
// Created by engin on 23.09.2019.
//

#ifndef LIMONENGINE_RENDERMETHODINTERFACE_H
#define LIMONENGINE_RENDERMETHODINTERFACE_H

#include <map>
#include <string>
#include <vector>

#include "LimonAPI.h"
#include "GraphicsProgram.h"

class GraphicsInterface;

class RenderMethodInterface {
/**
 * On shared library load, void registerAsTrigger(std::map<std::string, TriggerInterface*(*)(LimonAPI*)>*) function should be callable.
 *
 * This method will be called after object load.
 * For each trigger, the name of trigger and a constructor should be put in the map.
 */
    static std::map<std::string, RenderMethodInterface *(*)(GraphicsInterface *)> *typeMap;
protected:
    GraphicsInterface *graphicsInterface = nullptr;
    static std::map<std::string, RenderMethodInterface * (*)(GraphicsInterface * )> *getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order
        if (!typeMap) {
            typeMap = new std::map<std::string, RenderMethodInterface * (*)(GraphicsInterface * )>();
        }
        return typeMap;
    }

    RenderMethodInterface(GraphicsInterface
    * graphicsInterface):
            graphicsInterface(graphicsInterface) {}

public:
    // Not virtual
    static std::vector<std::string> getTriggerNames() {
        std::vector<std::string> names;
        for (auto it = typeMap->begin(); it != typeMap->end(); it++) {
            names.push_back(it->first);
        }
        return names;
    }

    static void registerType(const std::string &typeName, RenderMethodInterface *(*constructor)(GraphicsInterface *)) {
        (*getMap())[typeName] = constructor;
    }

    virtual std::vector<LimonAPI::ParameterRequest> getParameters() const = 0;

    virtual bool initRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonAPI::ParameterRequest> parameters) = 0;

    virtual bool renderFrame(std::shared_ptr<GraphicsProgram> program, std::vector<LimonAPI::ParameterRequest> parameters) = 0;

    virtual bool cleanupRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonAPI::ParameterRequest> parameters) = 0;

    virtual ~RenderMethodInterface() = default;

    virtual std::string getName() const = 0;

    static RenderMethodInterface *createRenderMethod(std::string const &s, GraphicsInterface *graphicsInterface) {
        std::map<std::string, RenderMethodInterface * (*)(GraphicsInterface * )>::iterator it = getMap()->find(s);
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
    virtual std::vector<LimonAPI::ParameterRequest> getParameters() const override {
        return std::vector<LimonAPI::ParameterRequest>();
    };

    virtual bool initRender(std::shared_ptr<GraphicsProgram> program [[gnu::unused]], std::vector<LimonAPI::ParameterRequest> parameters [[gnu::unused]]) {return false;};

    virtual bool renderFrame(std::shared_ptr<GraphicsProgram> program [[gnu::unused]], std::vector<LimonAPI::ParameterRequest> parameters [[gnu::unused]]) {return false;};

    virtual bool cleanupRender(std::shared_ptr<GraphicsProgram> program [[gnu::unused]], std::vector<LimonAPI::ParameterRequest> parameters [[gnu::unused]]) {return false;};

    std::string getName() const override {
        return "This object is not meant to be used";
    }


public:
    RenderMethodRegister(std::string const &s) : RenderMethodInterface(nullptr) {
        getMap()->insert(std::make_pair(s, &createT<T>));
    }


};


#endif //LIMONENGINE_RENDERMETHODINTERFACE_H
