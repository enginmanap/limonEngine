//
// Created by engin on 24.03.2016.
//

#ifndef UBERGAME_GUIRENDERABLE_H
#define UBERGAME_GUIRENDERABLE_H


#include "Renderable.h"

class GUIRenderable : public Renderable {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> textureCoordinates;
protected:
    //TODO maybe this should not be protected, but private
    std::vector<glm::mediump_uvec3> faces;
    GLuint textureID;
public:
    GUIRenderable(GLHelper *glHelper);

    /**
     * the position on x,y coordinates, and clockwise rotation as radian
     */
    void set2dWorldTransform(const glm::vec2 &position, const float rotation) {
        translate = glm::vec3(position, 0);
        orientation = glm::quat(cos(rotation / 2), 0, 0, -1 * sin(rotation / 2));
        isRotated = this->orientation.w < cos(0.1f / 2); //if the total rotation is less than 0.1 rad
        isDirty = true;
    }

    void setScale(float height, float width) {
        scale.x *= width;
        scale.y *= height;
    }

    virtual void render(Light *light);

    virtual void renderDebug();

    float getWidth() { return scale.x; }

    float getHeight() { return scale.y; }
};


#endif //UBERGAME_GUIRENDERABLE_H
