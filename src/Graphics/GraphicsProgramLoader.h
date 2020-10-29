//
// Created by engin on 27.04.2020.
//

#ifndef LIMONENGINE_GRAPHICSPROGRAMLOADER_H
#define LIMONENGINE_GRAPHICSPROGRAMLOADER_H


#include <tinyxml2.h>
#include <API/Graphics/GraphicsProgram.h>

class GraphicsProgramLoader {
public:
    static bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, const std::shared_ptr<GraphicsProgram> graphicsProgram);
    static std::shared_ptr<GraphicsProgram> deserialize(tinyxml2::XMLElement *programNode, std::shared_ptr<AssetManager> assetManager);
};


#endif //LIMONENGINE_GRAPHICSPROGRAMLOADER_H
