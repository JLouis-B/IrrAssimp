#include "IrrAssimp.h"

#include <iostream>

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

scene::ISkinnedMesh::SJoint* IrrAssimp::findJoint (scene::ISkinnedMesh* mesh, core::stringc jointName)
{
    for (unsigned int i = 0; i < mesh->getJointCount(); ++i)
    {
        if (core::stringc(mesh->getJointName(i)) == jointName)
            return mesh->getAllJoints()[i];
    }
    std::cout << "Error, no joint" << std::endl;
    return 0;
}

void IrrAssimp::createNode(scene::ISkinnedMesh* mesh, aiNode* node)
{
    scene::ISkinnedMesh::SJoint* jointParent = 0;

    if (node->mParent != 0)
    {
        jointParent = findJoint(mesh, node->mParent->mName.C_Str());
    }

    scene::ISkinnedMesh::SJoint* joint = mesh->addJoint(jointParent);
    joint->Name = node->mName.C_Str();

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        aiNode* childNode = node->mChildren[i];
        createNode(mesh, childNode);
    }
}


irr::scene::IAnimatedMesh* IrrAssimp::loadMesh(irr::core::stringc path)
{
    Assimp::Importer Importer;
    const aiScene* pScene = Importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

    if (!pScene)
        return 0;

    core::stringc fileDir = FileSystem->getFileDir(path);

    Mats.clear();

    // Create mesh
    scene::ISkinnedMesh* mesh = Smgr->createSkinnedMesh();

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

            if (FileSystem->existFile(path.C_Str()))
                irrMat.material.setTexture(0, Smgr->getVideoDriver()->getTexture(path.C_Str()));
            else
                irrMat.material.setTexture(0, Smgr->getVideoDriver()->getTexture(fileDir + "/" + path.C_Str()));
        }
        Mats.push_back(irrMat);
    }

    aiNode* root = pScene->mRootNode;
    createNode(mesh, root);

    for (unsigned int i = 0; i < pScene->mNumMeshes; ++i)
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
            const aiFace face = paiMesh->mFaces[j];

            buffer->Indices[3*j] = face.mIndices[0];
            buffer->Indices[3*j + 1] = face.mIndices[1];
            buffer->Indices[3*j + 2] = face.mIndices[2];
        }

        buffer->Material = Mats[paiMesh->mMaterialIndex].material;
        buffer->recalculateBoundingBox();

        if (!paiMesh->HasNormals())
            Smgr->getMeshManipulator()->recalculateNormals(buffer);

        for (unsigned int j = 0; j < paiMesh->mNumBones; ++j)
        {
            aiBone* bone = paiMesh->mBones[j];
            scene::ISkinnedMesh::SJoint* joint = findJoint(mesh, core::stringc(bone->mName.C_Str()));
            if (joint == 0)
                std::cout << "Error, no joint" << std::endl;

            core::matrix4 boneTransform;

            boneTransform[0] = bone->mOffsetMatrix.a1;
            boneTransform[1] = bone->mOffsetMatrix.b1;
            boneTransform[2] = bone->mOffsetMatrix.c1;
            boneTransform[3] = bone->mOffsetMatrix.d1;

            boneTransform[4] = bone->mOffsetMatrix.a2;
            boneTransform[5] = bone->mOffsetMatrix.b2;
            boneTransform[6] = bone->mOffsetMatrix.c2;
            boneTransform[7] = bone->mOffsetMatrix.d2;

            boneTransform[8] = bone->mOffsetMatrix.a3;
            boneTransform[9] = bone->mOffsetMatrix.b3;
            boneTransform[10] = bone->mOffsetMatrix.c3;
            boneTransform[11] = bone->mOffsetMatrix.d3;

            boneTransform[12] = bone->mOffsetMatrix.a4;
            boneTransform[13] = bone->mOffsetMatrix.b4;
            boneTransform[14] = bone->mOffsetMatrix.c4;
            boneTransform[15] = bone->mOffsetMatrix.d4;

            boneTransform.setInverseTranslation(boneTransform.getTranslation());

            joint->GlobalMatrix = boneTransform;
            joint->LocalMatrix = boneTransform;

            /*
            std::cout << "Irrlicht matrix" << std::endl << "--------------------------" << std::endl << "Translation = " << joint->LocalMatrix.getTranslation().X << ", " << joint->LocalMatrix.getTranslation().Y << ", " << joint->LocalMatrix.getTranslation().Z << std::endl
            << "Rotation = " << joint->LocalMatrix.getRotationDegrees().X << ", " << joint->LocalMatrix.getRotationDegrees().Y << ", " << joint->LocalMatrix.getRotationDegrees().Z << std::endl
            << "Scale = " << joint->LocalMatrix.getScale().X << ", " << joint->LocalMatrix.getScale().Y << ", " << joint->LocalMatrix.getScale().Z << std::endl << std::endl;
            */


            // Debug, display assimp matrix
            /*
            aiVector3D pos, scale;
            aiQuaternion rot;
            bone->mOffsetMatrix.Decompose(scale, rot, pos);


            std::cout << "Assimp matrix" << std::endl << "----------------------------" << std::endl << "Translation = " << pos.x << ", " << pos.y << ", " << pos.z << std::endl
            //<<"Rotation = " << rot << ", " << << ", " << << std::endl
            << "Scale = " << scale.x << ", " << scale.y << ", " << scale.z << std::endl << std::endl;
            */

            for (unsigned int h = 0; h < bone->mNumWeights; ++h)
            {
                aiVertexWeight weight = bone->mWeights[h];
                scene::ISkinnedMesh::SWeight* w = mesh->addWeight(joint);
                w->buffer_id = mesh->getMeshBufferCount() - 1;
                w->strength = weight.mWeight;
                w->vertex_id = weight.mVertexId;
            }

        }
    }

    // Compute the globals from locals
    for (int i = 0; i < mesh->getJointCount(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        computeLocal(mesh, pScene, joint);
    }


    //std::cout << "animation count : " << pScene->mNumAnimations << std::endl;
    for (unsigned int i = 0; i < pScene->mNumAnimations; ++i)
    {
        aiAnimation* anim = pScene->mAnimations[i];

        if (anim->mTicksPerSecond != 0)
            mesh->setAnimationSpeed(anim->mTicksPerSecond);

        //std::cout << "numChannels : " << anim->mNumChannels << std::endl;
        for (unsigned int j = 0; j < anim->mNumChannels; ++j)
        {
            aiNodeAnim* nodeAnim = anim->mChannels[j];
            scene::ISkinnedMesh::SJoint* joint = findJoint(mesh, nodeAnim->mNodeName.C_Str());

            for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mPositionKeys[k];

                scene::ISkinnedMesh::SPositionKey* irrKey = mesh->addPositionKey(joint);

                irrKey->frame = key.mTime; // TOFIX
                irrKey->position = core::vector3df(key.mValue.x, key.mValue.y, key.mValue.z);

                //std::cout << "PositionKey, Frame=" << anim->mTicksPerSecond << ", value=" << irrKey->position.X << ", " << irrKey->position.Y << ", " << irrKey->position.Z << std::endl;
            }
            for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; ++k)
            {
                aiQuatKey key = nodeAnim->mRotationKeys[k];
                aiQuaternion assimpQuat = key.mValue;

                core::quaternion quat (assimpQuat.x, assimpQuat.y, assimpQuat.z, assimpQuat.w);

                core::vector3df eulerRot;
                quat.toEuler(eulerRot);
                quat.set(-eulerRot);

                scene::ISkinnedMesh::SRotationKey* irrKey = mesh->addRotationKey(joint);

                irrKey->frame = key.mTime; // TOFIX
                irrKey->rotation = quat;

                //std::cout << "RotationKey, Frame=" << irrKey->frame << ", value=" << irrKey->rotation.X << ", " << irrKey->rotation.Y << ", " << irrKey->rotation.Z << ", " << irrKey->rotation.Z << std::endl;

            }
            for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mScalingKeys[k];

                scene::ISkinnedMesh::SScaleKey* irrKey = mesh->addScaleKey(joint);

                irrKey->frame = key.mTime;
                irrKey->scale = core::vector3df(key.mValue.x, key.mValue.y, key.mValue.z);

                //       std::cout << "ScaleKey, Frame=" << irrKey->frame << ", value=" << irrKey->scale.X << ", " << irrKey->scale.Y << ", " << irrKey->scale.Z << std::endl;
            }
        }

    }

    mesh->setDirty();
    mesh->finalize();

    return mesh;
}


void IrrAssimp::computeLocal(scene::ISkinnedMesh* mesh, const aiScene* pScene, scene::ISkinnedMesh::SJoint* joint)
{

    scene::ISkinnedMesh::SJoint* jointParent = 0;

    if(pScene->mRootNode->FindNode(aiString(joint->Name.c_str()))->mParent != NULL)
        jointParent = findJoint(mesh, pScene->mRootNode->FindNode(aiString(joint->Name.c_str()))->mParent->mName.C_Str());

    if (jointParent)
    {
        if (jointParent->LocalMatrix == jointParent->GlobalMatrix)
            computeLocal(mesh, pScene, jointParent);

        irr::core::matrix4 localParent = jointParent->GlobalMatrix;
        irr::core::matrix4 invLocalParent;
        localParent.getInverse(invLocalParent);

        joint->LocalMatrix = joint->GlobalMatrix * invLocalParent;
    }
    else
        ;   //std::cout << "Root ?" << std::endl;
    // -----------------------------------------------------------------
}

bool IrrAssimp::isLoadable(irr::core::stringc path)
{
    Assimp::Importer Importer;

    irr::core::stringc extension;
    irr::core::getFileNameExtension(extension, path);
    return Importer.IsExtensionSupported (extension.c_str());
}
