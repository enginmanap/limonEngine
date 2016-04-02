//
// Created by engin-evam on 29.03.2016.
//

#ifndef UBERGAME_SDLFONTMANAGER_H
#define UBERGAME_SDLFONTMANAGER_H

#include <iostream>
#include <string>
#include <map>

#include <SDL2/SDL_ttf.h>
class SDLFontManager {
    std::map<std::string, TTF_Font*> fonts;
    static const std::string DEFAULT_FONT_PATH;
    static const int DEFAULT_FONT_SIZE = 32;
public:
    SDLFontManager(){
        if(TTF_Init() == -1) {
            std::cerr << "SDL ttf could not init, error: \n"<< TTF_GetError() << "\n Exiting.." << std::endl;
            exit(1);
        } else {
            std::cout << "Font manager started" << std::endl;
        }
    }

    TTF_Font* getFont(const std::string fontPath, const int size){
        if(fonts.count(fontPath + std::to_string(size)) == 0){
            TTF_Font* ttf_font = TTF_OpenFont(fontPath.c_str(), size);

            if (!ttf_font) {
                std::cerr << "Could not create font with path=[" << fontPath << "]" << " and size=[" << size << "]"
                << "Error=" << std::endl << TTF_GetError()
                << "Creating with default path=[" << DEFAULT_FONT_PATH << "] and size=[" << DEFAULT_FONT_SIZE << "]" << std::endl;

                ttf_font = TTF_OpenFont(DEFAULT_FONT_PATH.c_str(), DEFAULT_FONT_SIZE);
            }

            fonts[fontPath + std::to_string(size)] = ttf_font;
        }

        return fonts[fontPath + std::to_string(size)];
    }

    ~SDLFontManager() {
        for(std::map<std::string, TTF_Font*>::iterator iter = fonts.begin(); iter != fonts.end(); ++iter){
            TTF_CloseFont((*iter).second);
        }
        TTF_Quit();

    }


};


#endif //UBERGAME_SDLFONTMANAGER_H
