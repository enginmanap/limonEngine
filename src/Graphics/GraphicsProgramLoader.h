//
// Created by engin on 27.04.2020.
//

#ifndef LIMONENGINE_GRAPHICSPROGRAMLOADER_H
#define LIMONENGINE_GRAPHICSPROGRAMLOADER_H


#include <map>
#include <tinyxml2.h>
#include <limonAPI/Graphics/GraphicsProgram.h>

class GraphicsProgramLoader {
public:
    static bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, const std::shared_ptr<GraphicsProgram> graphicsProgram);
    //sampler presets are not applied, they are units from files saved before units were assigned at load
    static std::shared_ptr<GraphicsProgram> deserialize(tinyxml2::XMLElement *programNode, std::shared_ptr<AssetManager> assetManager, std::map<std::string, uint32_t>& legacySamplerUnits);
    static bool isSamplerType(Uniform::VariableTypes type);
};


#endif //LIMONENGINE_GRAPHICSPROGRAMLOADER_H
