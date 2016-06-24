//
// Created by engin-evam on 29.03.2016.
//

#include "FontManager.h"

const std::string FontManager::DEFAULT_FONT_PATH = "Data/Fonts/Helvetica-Normal.ttf";

FontManager::FontManager(GLHelper *glHelper) : glHelper(glHelper) {
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Freetype could not init, error: \n" << "error" << "\n Exiting.." << std::endl;
        exit(1);
    } else {
        std::cout << "Font manager started" << std::endl;
    }
}

Face *FontManager::getFont(const std::string fontPath, const int size) {
    if (fonts.count(fontPath + std::to_string(size)) == 0) {
        FT_Face face;
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            std::cerr << "Could not create font with path=[" << fontPath << "]" << std::endl
            << "Creating with default path=[" << DEFAULT_FONT_PATH << "]" << std::endl;

            //if path is broken, this can be broken too, we need better error handling
            FT_New_Face(ft, DEFAULT_FONT_PATH.c_str(), 0, &face);
        }

        fonts[fontPath + std::to_string(size)] = new Face(glHelper, fontPath, size, face);
        FT_Set_Pixel_Sizes(face, 0, size);
        //now we should calculate what we have


        unsigned int w = 0;
        unsigned int h = 0;

        for (unsigned int i = 0; i < 256; i++) {
            if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
                fprintf(stderr, "Loading character %c failed!\n", i);
                continue;
            }

            w = std::max(w, face->glyph->bitmap.width);
            h = std::max(h, face->glyph->bitmap.rows);

        }

        std::cout << "atlas h: " << h << ", w " << w << std::endl;

        //now we have the atlas
    }

    return fonts[fontPath + std::to_string(size)];
}

FontManager::~FontManager() {
    for (std::map<std::string, Face *>::iterator iter = fonts.begin(); iter != fonts.end(); ++iter) {
        delete iter->second;
    }
    FT_Done_FreeType(ft);
}