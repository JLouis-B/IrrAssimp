#include "IrrAssimpExport.h"
#include <iostream>

using namespace irr;

IrrAssimpExport::IrrAssimpExport()
{
    //ctor
}

IrrAssimpExport::~IrrAssimpExport()
{
    //dtor
}

void IrrAssimpExport::writeFile(irr::scene::IMesh* mesh, irr::core::stringc format, irr::core::stringc filename)
{
    Assimp::Exporter exporter;

    /*
    for (int i = 0; i < exporter.GetExportFormatCount(); ++i)
    {
         const aiExportFormatDesc* formatDesc = exporter.GetExportFormatDescription(i);
         std::cout << "id=" << formatDesc->id << ", extension=" << formatDesc->fileExtension << std::endl << "description= " << formatDesc->description << std::endl;
    }
    */

    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode();
    scene->mRootNode->mNumMeshes = mesh->getMeshBufferCount();
    scene->mRootNode->mMeshes = new unsigned int[mesh->getMeshBufferCount()];
    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
        scene->mRootNode->mMeshes[i] = i;

    scene->mNumMeshes = mesh->getMeshBufferCount();
    scene->mMeshes = new aiMesh*[scene->mNumMeshes];

    scene->mNumMaterials = scene->mNumMeshes;
    scene->mMaterials = new aiMaterial*[scene->mNumMeshes];

    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        aiMesh* assMesh = new aiMesh();
        irr::scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);

        assMesh->mNumVertices = buffer->getVertexCount();
        assMesh->mVertices = new aiVector3D[assMesh->mNumVertices];
        assMesh->mNormals = new aiVector3D[assMesh->mNumVertices];

        //assMesh->mTextureCoords = new aiVector3D;
        assMesh->mTextureCoords[0] = new aiVector3D[assMesh->mNumVertices];
        /*for (unsigned int j = 1; j < 8; ++j)
        {
            assMesh->mTextureCoords[j] = 0;
        }
        */

        assMesh->mNumUVComponents[0] = 2;
        /*
        for (unsigned int j = 1; j < 8; ++j)
        {
            assMesh->mNumUVComponents[j] = 0;
        }
        */
        for (unsigned int j = 0; j < buffer->getVertexCount(); ++j)
        {
            core::vector3df vertexPosition = buffer->getPosition(j);
            core::vector3df normal = buffer->getNormal(j);
            core::vector2df uv = buffer->getTCoords(j);

            assMesh->mVertices[j] = aiVector3D(vertexPosition.X, vertexPosition.Y, vertexPosition.Z);
            assMesh->mNormals[j] = aiVector3D(normal.X, normal.Y, normal.Z);

            assMesh->mTextureCoords[0][j] = aiVector3D(uv.X, uv.Y, 0);
        }

        assMesh->mNumFaces = buffer->getIndexCount() / 3;
        assMesh->mFaces = new aiFace[assMesh->mNumFaces];
        for (unsigned int j = 0; j < assMesh->mNumFaces; ++j)
        {
            aiFace face;
            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3];
            face.mIndices[0] = buffer->getIndices()[3 * j + 0];
            face.mIndices[1] = buffer->getIndices()[3 * j + 1];
            face.mIndices[2] = buffer->getIndices()[3 * j + 2];
            assMesh->mFaces[j] = face;
        }

        scene->mMaterials[i] = new aiMaterial();
        scene->mMaterials[i]->mNumProperties = 0;
        assMesh->mMaterialIndex = i;

        video::SMaterial mat = buffer->getMaterial();
        if (mat.getTexture(0))
        {
            aiString textureName = aiString(mat.getTexture(0)->getName().getPath().c_str());
            scene->mMaterials[i]->AddProperty(&textureName, AI_MATKEY_TEXTURE_DIFFUSE(0));
        }

        scene->mMeshes[i] = assMesh;
    }

    exporter.Export(scene, format.c_str(), filename.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
}

