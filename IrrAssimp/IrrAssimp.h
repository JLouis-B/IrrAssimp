#ifndef IRRASSIMP_H
#define IRRASSIMP_H

#include <irrlicht.h>
#include "IrrAssimpExport.h"
#include "IrrAssimpImport.h"

#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags
#include <assimp/Importer.hpp>


class IrrAssimp
{
    public:
        IrrAssimp(irr::scene::ISceneManager* smgr);
        virtual ~IrrAssimp();

        irr::scene::IAnimatedMesh* getMesh(const irr::io::path& path);

        bool exportMesh(irr::scene::IMesh* mesh, irr::core::stringc format, irr::core::stringc path);

        bool isLoadable(irr::core::stringc path);

    protected:
    private:
        irr::scene::IMeshCache* Cache;
        irr::io::IFileSystem* FileSystem;
        irr::scene::ISceneManager* Smgr;

        IrrAssimpImport* Importer;
        IrrAssimpExport* Exporter;

};

#endif // IRRASSIMP_H
