#ifndef LIMONENGINE_RENDERPROFILESCOPE_H
#define LIMONENGINE_RENDERPROFILESCOPE_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "tracy/Tracy.hpp"
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "Utils/HashUtil.h"

class GraphicsPipelineStage;
class GraphicsProgram;
class Light;

class RenderProfileScope {
    enum class Category { STAGE, METHOD, LIGHT };

    GraphicsInterface* graphicsWrapper = nullptr;
    GraphicsInterface::RenderStats statsAtBegin;
    bool active = false;//decided once at begin, so toggling tracing inside the scope still ends the GPU zone
#ifdef TRACY_ENABLE
    std::optional<tracy::ScopedZone> cpuZone;//carries the text, GPU zones can't have any
#endif

    void begin(Category category, const std::string& label, const std::string& text);

public:
    RenderProfileScope(GraphicsInterface* graphicsWrapper, const GraphicsPipelineStage& stage, bool clear);
    RenderProfileScope(GraphicsInterface* graphicsWrapper, const std::string& methodName, const GraphicsProgram* program,
                       const std::string& cameraName, const std::vector<HashUtil::HashedString>& tags);
    RenderProfileScope(GraphicsInterface* graphicsWrapper, const Light& light, uint32_t layer);
    ~RenderProfileScope();

    RenderProfileScope(const RenderProfileScope&) = delete;
    RenderProfileScope& operator=(const RenderProfileScope&) = delete;
};

#endif //LIMONENGINE_RENDERPROFILESCOPE_H
