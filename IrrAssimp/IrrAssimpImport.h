#ifndef IRRASSIMPIMPORT_H
#define IRRASSIMPIMPORT_H


#include <irrlicht.h>

#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags
#include <assimp/Importer.hpp>


struct Material
{
    unsigned int id;
    irr::video::SMaterial material;
};


class IrrAssimpImport
{
    public:
        IrrAssimpImport(irr::scene::ISceneManager* smgr);
        virtual ~IrrAssimpImport();

        irr::scene::IAnimatedMesh* loadMesh(irr::core::stringc path);

    protected:
    private:
        void createNode(irr::scene::ISkinnedMesh* mesh, aiNode* node);
        irr::scene::ISkinnedMesh::SJoint* findJoint (irr::scene::ISkinnedMesh* mesh, irr::core::stringc jointName);
        aiNode* findNode (const aiScene* scene, aiString jointName);
        void computeLocal(irr::scene::ISkinnedMesh* mesh, const aiScene* pScene, irr::scene::ISkinnedMesh::SJoint* joint);

        irr::core::array<Material> Mats;

        irr::scene::ISceneManager* Smgr;
        irr::io::IFileSystem* FileSystem;
};

#endif // IRRASSIMPIMPORT_H
