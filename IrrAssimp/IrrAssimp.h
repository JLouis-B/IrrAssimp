#ifndef IRRASSIMP_H
#define IRRASSIMP_H

#include <irrlicht.h>

#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags
#include <assimp/Importer.hpp>


struct Material
{
    unsigned int id;
    irr::video::SMaterial material;
};

class IrrAssimp
{
    public:
        IrrAssimp(irr::scene::ISceneManager* smgr);
        virtual ~IrrAssimp();

        irr::scene::IAnimatedMesh* getMesh(const irr::io::path& path);
        irr::scene::IAnimatedMesh* loadMesh(irr::core::stringc path);

        bool isLoadable(irr::core::stringc path);

    protected:
    private:
        irr::scene::IMeshCache* Cache;
        irr::io::IFileSystem* FileSystem;
        irr::scene::ISceneManager* Smgr;

        irr::core::array<Material> Mats;
};

#endif // IRRASSIMP_H
