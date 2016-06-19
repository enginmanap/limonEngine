//
// Created by engin-evam on 29.03.2016.
//

#ifndef UBERGAME_SDLFONTMANAGER_H
#define UBERGAME_SDLFONTMANAGER_H

#include <iostream>
#include <string>
#include <map>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include "glm/detail/type_vec.hpp"
#include "GLHelper.h"


class Glyph {
    GLuint textureID;
    glm::mediump_vec2 size;
    glm::mediump_vec2 bearing;
    GLuint advance;
public:
    Glyph(GLHelper *glHelper, FT_Face face, const int size, const char character) :
            textureID(0), size(glm::mediump_vec2(0)), bearing(glm::mediump_vec2(0)), advance(0) {
        //FIXME this is not correct, there is a better function in API
        FT_Set_Pixel_Sizes(face, 0, size);
        if (FT_Load_Char(face, character, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
        } else {
            textureID = glHelper->loadTexture(face->glyph->bitmap.rows, face->glyph->bitmap.width, GL_RED,
                                              face->glyph->bitmap.buffer);
            this->size = glm::mediump_vec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            bearing = glm::mediump_vec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
            advance = face->glyph->advance.x;
            /**
            std::cout << "Glyph load success. Information:" << std::endl
            << "char: " << character << std::endl
            << "size: " << this->size.x << "," << this->size.y << std::endl
            << "bearing: " << bearing.x << "," << bearing.y << std::endl
            << "advance: " << advance << " (" << advance / 64 << ")" << std::endl;
             */
        }

    }

    GLuint getTextureID() const { return textureID; }

    const glm::mediump_vec2 &getSize() const { return size; }

    const glm::mediump_vec2 &getBearing() const { return bearing; }

    GLuint getAdvance() const { return advance; }
};

class Face {
    GLHelper *glHelper;
    std::string path;
    unsigned int size;
    FT_Face face;
    std::map<const char, Glyph *> glyphs;
public:
    Face(GLHelper *glHelper, std::string path, int size, FT_Face face) : glHelper(glHelper), path(path), size(size),
                                                                         face(face) { }

    const Glyph *getGlyph(const char character) {
        if (glyphs.count(character) == 0) {
            glyphs[character] = new Glyph(glHelper, face, size, character);
        }
        return glyphs[character];
    }

    ~Face() {
        for (std::map<const char, Glyph *>::iterator iter = glyphs.begin(); iter != glyphs.end(); ++iter) {
            delete iter->second;
        }
    }
};

class FontManager {
    GLHelper *glHelper;
    std::map<std::string, Face *> fonts;
    static const std::string DEFAULT_FONT_PATH;
    static const int DEFAULT_FONT_SIZE = 32;
    FT_Library ft;
public:
    FontManager(GLHelper *glHelper);

    Face *getFont(const std::string fontPath, const int size);

    ~FontManager();
};


#endif //UBERGAME_SDLFONTMANAGER_H
