//
// Created by engin on 24.03.2016.
//

#ifndef LIMONENGINE_GUIRENDERABLE_H
#define LIMONENGINE_GUIRENDERABLE_H


#include "../Renderable.h"
#include "../BulletDebugDrawer.h"

class GUILayer;

class GUIRenderable : public Renderable {
    std::vector<GUILayer*> parentLayers;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> textureCoordinates;
    uint32_t worldID;

protected:
    //TODO maybe this should not be protected, but private
    std::vector<glm::mediump_uvec3> faces;

    std::string name;
    GLuint textureID;

public:
    explicit GUIRenderable(GLHelper *glHelper, uint32_t id);

    void addedToLayer(GUILayer* layer);

    virtual ~GUIRenderable();
    /**
     * the position on x,y coordinates, and clockwise rotation as radian
     */
    void set2dWorldTransform(const glm::vec2 &position, const float rotation) {
        transformation.setTranslate(glm::vec3(position, 0));
        transformation.setOrientation(glm::quat(cos(rotation / 2), 0, 0, -1 * sin(rotation / 2)));
    }

    /**
     * Adds translate to current translation
     * @param translate
     */
    void addTranslate(const glm::vec2 &translate) {
        transformation.addTranslate(glm::vec3(translate, 0));
    }

    /**
     * Sets translate to given translation. It overrides old one.
     * @param translate
     */
    void setTranslate(const glm::vec2 &translate) {
        transformation.setTranslate(glm::vec3(translate, 0));
    }

    glm::vec2 getTranslate() const {
        return glm::vec2(transformation.getTranslate().x, transformation.getTranslate().y);
    }

    void setScale(float height, float width) {
        transformation.setScale(glm::vec3(width, height, 0.0f));
    }

    virtual void renderDebug(BulletDebugDrawer *debugDrawer);

    virtual void setupForTime(long time __attribute__((unused))) {};//Most of the GUI elements shouldn't care about the time, so we can put an empty implementation

    float getWidth() { return transformation.getScale().x; }

    float getHeight() { return transformation.getScale().y; }

    uint32_t getWorldID() {
        return worldID;
    }

    const std::string& getName() {
        return name;
    }
};


#endif //LIMONENGINE_GUIRENDERABLE_H
