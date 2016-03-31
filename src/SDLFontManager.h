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
            fonts[fontPath + std::to_string(size)] = TTF_OpenFont(fontPath.c_str(), size);
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
