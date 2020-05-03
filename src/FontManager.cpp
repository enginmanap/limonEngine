//
// Created by engin-evam on 29.03.2016.
//

#include "FontManager.h"

const std::string FontManager::DEFAULT_FONT_PATH = "Data/Fonts/Helvetica-Normal.ttf";

FontManager::FontManager(GraphicsInterface* graphicsWrapper) : graphicsWrapper(graphicsWrapper) {
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Freetype could not init, error: \n" << "error" << "\n Exiting.." << std::endl;
        exit(1);
    } else {
        std::cout << "Font manager started" << std::endl;
    }
}

Face *FontManager::getFont(const std::string &fontPath, const uint32_t size) {
    std::pair<std::string, uint32_t> fontDescription = std::make_pair(fontPath, size);
    if (fonts.find(fontDescription) == fonts.end()) {
        FT_Face face;
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            std::cerr << "Could not create font with path=[" << fontPath << "]" << std::endl
            << "Creating with default path=[" << DEFAULT_FONT_PATH << "]" << std::endl;

            //if path is broken, this can be broken too, we need better error handling
            FT_New_Face(ft, DEFAULT_FONT_PATH.c_str(), 0, &face);
        }

        fonts[fontDescription] = new Face(graphicsWrapper, fontPath, size, face);
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

        //now we have maximum size of the textures
    }

    return fonts[fontDescription];
}

FontManager::~FontManager() {
    for (auto iter = fonts.begin(); iter != fonts.end(); ++iter) {
        delete iter->second;
    }
    FT_Done_FreeType(ft);
}