//
// Created by Engin Manap on 1.03.2016.
//

#ifndef LIMONENGINE_SKYBOX_H
#define LIMONENGINE_SKYBOX_H

#include <string>

#include "../Renderable.h"
#include "../Assets/CubeMapAsset.h"
#include "../Assets/AssetManager.h"

class SkyBox : public Renderable, public GameObject {
    uint32_t objectID;
    AssetManager *assetManager;
    std::string path, right, left, top, down, back, front;
public:
    const std::string &getPath() const;

    const std::string &getRight() const;

    const std::string &getLeft() const;

    const std::string &getTop() const;

    const std::string &getDown() const;

    const std::string &getBack() const;

    const std::string &getFront() const;

private:
    std::vector<glm::vec3> vertices;
    std::vector<glm::mediump_uvec3> faces;

    CubeMapAsset *cubeMap;

public:
    SkyBox(uint32_t objectID, AssetManager *assetManager, std::string path, std::string right, std::string left,
           std::string top, std::string down, std::string back, std::string front);

    void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram) override;

    void setupForTime(long time __attribute__((unused))) {};

    ~SkyBox() {
        assetManager->freeAsset(cubeMap->getNames());
    }

    /************Game Object methods **************/
    uint32_t getWorldObjectID() const override {
        return objectID;
    };

    ObjectTypes getTypeID() const {
        return GameObject::SKYBOX;
    }

    std::string getName() const {
        return path;
    };
    /************Game Object methods **************/

};

#endif //LIMONENGINE_SKYBOX_H
