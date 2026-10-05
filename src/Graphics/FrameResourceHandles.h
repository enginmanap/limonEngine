#ifndef LIMONENGINE_FRAMERESOURCEHANDLES_H
#define LIMONENGINE_FRAMERESOURCEHANDLES_H

#include <cstdint>

//GameEngine owns these and every world writes into them, see GraphicsInterface::swapFrameResources
struct FrameResourceHandles {
    uint32_t modelTransformTexture = 0;
    uint32_t boneTransformTexture = 0;
    uint32_t lightBlockBuffer = 0;
    uint32_t playerBlockBuffer = 0;
};

#endif //LIMONENGINE_FRAMERESOURCEHANDLES_H
