//
// Created by engin on 27.07.2016.
//

#ifndef UBERGAME_ASSET_H
#define UBERGAME_ASSET_H

#include <vector>
#include <string>
#include "../GLHelper.h"

class Asset {

protected:
    /**
     * This is an empty constructor, used to indicate what parameters the Asset constructors should have
     * @param glHelper pointer to render subsystem
     * @param fileList Asset files to load
     * @return empty asset
     */
    Asset(GLHelper *glHelper, const std::vector<std::string> &fileList) {};
};


#endif //UBERGAME_ASSET_H
