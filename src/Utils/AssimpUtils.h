//
// Created by engin on 16.05.2016.
//

#ifndef UBERGAME_ASSIMPUTILS_H
#define UBERGAME_ASSIMPUTILS_H

#include <assimp/scene.h>

#define aisgl_min(x, y) (x<y?x:y)
#define aisgl_max(x, y) (y>x?y:x)


class AssimpUtils {

    static void get_bounding_box_for_node(const aiScene *scene, const aiNode *nd,
                                          aiVector3D *min,
                                          aiVector3D *max) {
        aiMatrix4x4 prev;

        for (unsigned int n = 0; n < nd->mNumMeshes; ++n) {
            const aiMesh *mesh = scene->mMeshes[nd->mMeshes[n]];
            for (unsigned int t = 0; t < mesh->mNumVertices; ++t) {

                aiVector3D tmp = mesh->mVertices[t];

                min->x = aisgl_min(min->x, tmp.x);
                min->y = aisgl_min(min->y, tmp.y);
                min->z = aisgl_min(min->z, tmp.z);

                max->x = aisgl_max(max->x, tmp.x);
                max->y = aisgl_max(max->y, tmp.y);
                max->z = aisgl_max(max->z, tmp.z);
            }
        }

        for (unsigned int n = 0; n < nd->mNumChildren; ++n) {
            get_bounding_box_for_node(scene, nd->mChildren[n], min, max);
        }
    }

public:
    static void get_bounding_box(const aiScene *scene, aiVector3D *min, aiVector3D *max) {
        min->x = min->y = min->z = 1e10f;
        max->x = max->y = max->z = -1e10f;
        get_bounding_box_for_node(scene, scene->mRootNode, min, max);
    }

};


#endif //UBERGAME_ASSIMPUTILS_H
