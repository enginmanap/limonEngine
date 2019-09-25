//
// Created by engin on 22.09.2019.
//

#ifndef LIMONENGINE_QUADRENDERER_H
#define LIMONENGINE_QUADRENDERER_H

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <memory>

class GraphicsInterface;
class GraphicsProgram;

class QuadRender {
    uint_fast32_t vao, ebo;
    std::vector<uint32_t> bufferObjects;

protected:
    GraphicsInterface* graphicsWrapper = nullptr;
public:
    QuadRender(GraphicsInterface* graphicsWrapper);

    void render(std::shared_ptr<GraphicsProgram> renderProgram);

};


#endif //LIMONENGINE_QUADRENDERER_H
