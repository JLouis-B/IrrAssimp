#ifndef IRRASSIMPIMPORT_H
#define IRRASSIMPIMPORT_H


#include <irrlicht.h>

#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags
#include <assimp/Importer.hpp>

#include "IrrAssimpUtils.h"

class SkinnedVertex
{
public:
    SkinnedVertex()
    {
        Moved = false;
        Position = irr::core::vector3df(0, 0, 0);
        Normal = irr::core::vector3df(0, 0, 0);
    }

    bool Moved;
    irr::core::vector3df Position;
    irr::core::vector3df Normal;
};

class IrrAssimpImport : public irr::scene::IMeshLoader
{
    public:
        IrrAssimpImport(irr::scene::ISceneManager* smgr);
        virtual ~IrrAssimpImport();

        virtual irr::scene::IAnimatedMesh* createMesh(irr::io::IReadFile* file);
        virtual bool isALoadableFileExtension(const irr::io::path& filename) const;

        irr::core::stringc Error;

    protected:
    private:
        void createNode(irr::scene::ISkinnedMesh* mesh, aiNode* node, bool isRoot);
        irr::scene::ISkinnedMesh::SJoint* findJoint (irr::scene::ISkinnedMesh* mesh, irr::core::stringc jointName);
        aiNode* findNode (const aiScene* scene, aiString jointName);
        irr::video::ITexture* getTexture(irr::core::stringc path, irr::core::stringc fileDir);

        irr::core::array<irr::video::SMaterial> Mats;

        irr::scene::ISceneManager* Smgr;
        irr::io::IFileSystem* FileSystem;

        irr::core::matrix4 InverseRootNodeWorldTransform;

        irr::core::array<SkinnedVertex> skinnedVertex;

        void skinJoint(irr::scene::ISkinnedMesh* mesh, irr::scene::ISkinnedMesh::SJoint *joint, aiBone* bone);
        void buildSkinnedVertexArray(irr::scene::IMeshBuffer* buffer);
        void applySkinnedVertexArray(irr::scene::IMeshBuffer* buffer);
};

#endif // IRRASSIMPIMPORT_H
