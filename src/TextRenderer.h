//
// Created by engin on 14.03.2016.
//

#ifndef UBERGAME_TEXTRENDERER_H
#define UBERGAME_TEXTRENDERER_H

#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "GLHelper.h"
#include "Renderable.h"

class TextRenderer : public Renderable {
    GLuint textureID;
    std::vector<glm::vec3> vertices;
    std::vector<glm::mediump_uvec3> faces;
    std::vector<glm::vec2> textureCoordinates;
public:
    TextRenderer(GLHelper* glHelper, const std::string fontFile, const std::string text, const int size, const glm::lowp_uvec3 color);
    /**
     * the position on x,y coordinates, and clockwise rotation as radian
     */
    void set2dWorldTransform(const glm::vec2 &position, const float rotation){
        translate = glm::vec3(position, 0);
        orientation = glm::quat(cos(rotation/2), 0,0,-1 * sin(rotation/2));
    }

    void setScale(float height, float width) {
        scale.x *= width;
        scale.y *= height;
    }

    ~TextRenderer() {
        TTF_Quit();
    }

    void updateText(std::string text){
        std::cerr << "text update is not implemented" << std::endl;
    }

    void render();

};


#endif //UBERGAME_TEXTRENDERER_H
