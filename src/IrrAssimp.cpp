#include "../include/IrrAssimp.h"

//#include <iostream>

using namespace irr;

IrrAssimp::IrrAssimp(irr::scene::ISceneManager* smgr)
{
    Cache = smgr->getMeshCache();
    FileSystem = smgr->getFileSystem();
    Smgr = smgr;
}

IrrAssimp::~IrrAssimp()
{
    //dtor
}

irr::scene::IAnimatedMesh* IrrAssimp::getMesh(const io::path& path)
{
	scene::IAnimatedMesh* msh = Cache->getMeshByName(path);
	if (msh)
		return msh;

	io::IReadFile* file = FileSystem->createAndOpenFile(path);
	if (!file)
	{
		//os::Printer::log("Could not load mesh, because file could not be opened: ", path, ELL_ERROR);
		return 0;
	}

	if (isLoadable(path))
    {
        msh = loadMesh(path);

        if (msh)
        {
            Cache->addMesh(path, msh);
            msh->drop();
        }
    }


	file->drop();

/*
	if (!msh)
		os::Printer::log("Could not load mesh, file format seems to be unsupported", filename, ELL_ERROR);
	else
		os::Printer::log("Loaded mesh", filename, ELL_INFORMATION);
*/

	return msh;
}

irr::scene::IAnimatedMesh* IrrAssimp::loadMesh(irr::core::stringc path)
{
    core::stringc dileDir = FileSystem->getFileDir(path);

    Mats.clear();

    // Create mesh
    scene::ISkinnedMesh* mesh = Smgr->createSkinnedMesh();

    Assimp::Importer Importer;
    const aiScene* pScene = Importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

    if (!pScene)
        return 0;

    for (int i = 0; i < pScene->mNumMaterials; ++i)
    {
        Material irrMat;
        irrMat.id = i;
        irrMat.material.MaterialType = video::EMT_SOLID;



        aiMaterial* mat = pScene->mMaterials[i];
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString path;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);

            if (Smgr->getVideoDriver()->getTexture(path.C_Str()))
                irrMat.material.setTexture(0, Smgr->getVideoDriver()->getTexture(path.C_Str()));
            else
                irrMat.material.setTexture(0, Smgr->getVideoDriver()->getTexture(dileDir + "/" + path.C_Str()));
        }
        Mats.push_back(irrMat);
    }


    for (int i = 0; i < pScene->mNumMeshes; ++i)
    {
        //std::cout << "i=" << i << " of " << pScene->mNumMeshes << std::endl;

        aiMesh* paiMesh = pScene->mMeshes[i];

        scene::SSkinMeshBuffer* buffer = mesh->addMeshBuffer();

        buffer->Vertices_Standard.reallocate(paiMesh->mNumVertices);
        buffer->Vertices_Standard.set_used(paiMesh->mNumVertices);

        for (unsigned int j = 0; j < paiMesh->mNumVertices; ++j)
        {
            aiVector3D vertex = paiMesh->mVertices[j];
            const aiVector3D* uv = paiMesh->mTextureCoords[0];

            buffer->Vertices_Standard[j].Pos.X = vertex.x;
            buffer->Vertices_Standard[j].Pos.Y = vertex.y;
            buffer->Vertices_Standard[j].Pos.Z = vertex.z;

            //std::cout << "Coords=" << buffer->Vertices_Standard[j].Pos.X << ", " << buffer->Vertices_Standard[j].Pos.Y << ", " << buffer->Vertices_Standard[j].Pos.Z << std::endl;

            buffer->Vertices_Standard[j].TCoords.X = uv[j].x;
            buffer->Vertices_Standard[j].TCoords.Y = uv[j].y;

            if (paiMesh->HasNormals())
            {
                buffer->Vertices_Standard[j].Normal.X = paiMesh->mNormals[j].x;
                buffer->Vertices_Standard[j].Normal.Y = paiMesh->mNormals[j].y;
                buffer->Vertices_Standard[j].Normal.Z = paiMesh->mNormals[j].z;
            }

            buffer->Vertices_Standard[j].Color = irr::video::SColor(255,255,255,255);
        }

        buffer->Indices.reallocate(paiMesh->mNumFaces * 3);
        buffer->Indices.set_used(paiMesh->mNumFaces * 3);


        for (unsigned int j = 0; j < paiMesh->mNumFaces; ++j)
        {
            aiFace face = paiMesh->mFaces[j];

            buffer->Indices[3*j] = face.mIndices[0];
            buffer->Indices[3*j + 1] = face.mIndices[1];
            buffer->Indices[3*j + 2] = face.mIndices[2];
        }

        buffer->Material = Mats[paiMesh->mMaterialIndex].material;

        /*
        video::SMaterial tmp;
        tmp.MaterialType = video::EMT_SOLID ;
        tmp.DiffuseColor = video::SColor(255,255,255,255);
        tmp.AmbientColor = video::SColor(255,255,255,255);
        tmp.EmissiveColor = video::SColor(255,255,255,255);
        buffer->Material = tmp;*/

        buffer->recalculateBoundingBox();

        if (!paiMesh->HasNormals())
            Smgr->getMeshManipulator()->recalculateNormals(buffer);
    }

    mesh->setDirty();
    mesh->finalize();

    return mesh;
}

bool IrrAssimp::isLoadable(irr::core::stringc path)
{
    Assimp::Importer Importer;

    irr::core::stringc extension;
    irr::core::getFileNameExtension(extension, path);
    return Importer.IsExtensionSupported (extension.c_str());
}
