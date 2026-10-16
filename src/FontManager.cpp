//
// Created by engin-evam on 29.03.2016.
//

#include "FontManager.h"
#include "LimonFileSystem.h"
#include "Package/EngineDataFiles.h"


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
        FT_Face face = nullptr;
        std::vector<uint8_t> fontData;
        if (!LimonFileSystem::readFile(fontPath, fontData) ||
            FT_New_Memory_Face(ft, fontData.data(), static_cast<FT_Long>(fontData.size()), 0, &face)) {
            std::cerr << "Could not create font with path=[" << fontPath << "]" << std::endl
            << "Creating with default path=[" << EngineDataFiles::DEFAULT_FONT << "]" << std::endl;

            //if path is broken, this can be broken too, we need better error handling
            LimonFileSystem::readFile(EngineDataFiles::DEFAULT_FONT, fontData);
            FT_New_Memory_Face(ft, fontData.data(), static_cast<FT_Long>(fontData.size()), 0, &face);
        }

        fonts[fontDescription] = new Face(graphicsWrapper, fontPath, size, face, std::move(fontData));
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