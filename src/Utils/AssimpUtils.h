//
// Created by engin on 16.05.2016.
//

#ifndef LIMONENGINE_ASSIMPUTILS_H
#define LIMONENGINE_ASSIMPUTILS_H

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>
#define aisgl_min(x, y) (x<y?x:y)
#define aisgl_max(x, y) (y>x?y:x)

/**
 * This class is taken from an official sample of Assimp at:
 *  assimp/samples/SimpleOpenGL/Sample_SimpleOpenGL.c
 * Because of that, I am avoiding unnecessary modifications like variable rename etc.
 */
class AssimpUtils {

    static void get_bounding_box_for_node (const aiScene *scene, const struct aiNode* nd,
                                    aiVector3D* min,
                                    aiVector3D* max,
                                    aiMatrix4x4* trafo
    ){
        aiMatrix4x4 prev;
        unsigned int n = 0, t;

        prev = *trafo;
        aiMultiplyMatrix4(trafo,&nd->mTransformation);

        for (; n < nd->mNumMeshes; ++n) {
            const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
            for (t = 0; t < mesh->mNumVertices; ++t) {

                aiVector3D tmp = mesh->mVertices[t];
                aiTransformVecByMatrix4(&tmp,trafo);

                min->x = aisgl_min(min->x,tmp.x);
                min->y = aisgl_min(min->y,tmp.y);
                min->z = aisgl_min(min->z,tmp.z);

                max->x = aisgl_max(max->x,tmp.x);
                max->y = aisgl_max(max->y,tmp.y);
                max->z = aisgl_max(max->z,tmp.z);
            }
        }

        for (n = 0; n < nd->mNumChildren; ++n) {
            get_bounding_box_for_node(scene, nd->mChildren[n],min,max,trafo);
        }
        *trafo = prev;
    }

public:
    static void get_bounding_box(const aiScene *scene, aiVector3D *min, aiVector3D *max) {
        aiMatrix4x4 trafo;
        aiIdentityMatrix4(&trafo);

        min->x = min->y = min->z = 1e10f;
        max->x = max->y = max->z = -1e10f;
        get_bounding_box_for_node(scene, scene->mRootNode, min, max, &trafo);
    }

};


#endif //LIMONENGINE_ASSIMPUTILS_H
