#include "IrrAssimp.h"

#include <iostream>

using namespace irr;

IrrAssimp::IrrAssimp(irr::scene::ISceneManager* smgr)
{
    Smgr = smgr;
    Cache = smgr->getMeshCache();
    FileSystem = smgr->getFileSystem();
}

IrrAssimp::~IrrAssimp()
{
    //dtor
}

irr::core::matrix4 AssimpToIrrMatrix(aiMatrix4x4 assimpMatrix)
{
    core::matrix4 irrMatrix;

    irrMatrix[0] = assimpMatrix.a1;
    irrMatrix[1] = assimpMatrix.b1;
    irrMatrix[2] = assimpMatrix.c1;
    irrMatrix[3] = assimpMatrix.d1;

    irrMatrix[4] = assimpMatrix.a2;
    irrMatrix[5] = assimpMatrix.b2;
    irrMatrix[6] = assimpMatrix.c2;
    irrMatrix[7] = assimpMatrix.d2;

    irrMatrix[8] = assimpMatrix.a3;
    irrMatrix[9] = assimpMatrix.b3;
    irrMatrix[10] = assimpMatrix.c3;
    irrMatrix[11] = assimpMatrix.d3;

    irrMatrix[12] = assimpMatrix.a4;
    irrMatrix[13] = assimpMatrix.b4;
    irrMatrix[14] = assimpMatrix.c4;
    irrMatrix[15] = assimpMatrix.d4;

    return irrMatrix;
}

aiMatrix4x4 getAbsoluteMatrixOf(aiNode* node)
{
    if (node->mParent == NULL)
        return node->mTransformation;

    return node->mTransformation * getAbsoluteMatrixOf(node->mParent);
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

aiNode* IrrAssimp::findNode (const aiScene* scene, aiString jointName)
{
    if (scene->mRootNode->mName == jointName)
        return scene->mRootNode;

    return scene->mRootNode->FindNode(jointName);
}


aiBone* findBone (const aiScene* scene, int meshID, aiString jointName)
{
    aiMesh* mesh = scene->mMeshes[meshID];
    for (unsigned int i = 0; i < mesh->mNumBones; ++i)
    {
        if (mesh->mBones[i]->mName == jointName)
            return mesh->mBones[i];
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
    joint->LocalMatrix = AssimpToIrrMatrix(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        joint->AttachedMeshes.push_back((unsigned int)mesh->getMeshBuffer(node->mMeshes[i]));

    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        aiNode* childNode = node->mChildren[i];

        createNode(mesh, childNode);
    }
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

void Log(core::vector3df vect)
{
    std::cout << "Vector = " << vect.X << ", " << vect.Y << ", " << vect.Z << std::endl;
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

    for (unsigned int i = 0; i < pScene->mNumMaterials; ++i)
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
            else if (FileSystem->existFile(fileDir + "/" + path.C_Str()))
                irrMat.material.setTexture(0, Smgr->getVideoDriver()->getTexture(fileDir + "/" + path.C_Str()));
            else if (FileSystem->existFile(fileDir + "/" + FileSystem->getFileBasename(path.C_Str())))
                irrMat.material.setTexture(0, Smgr->getVideoDriver()->getTexture(fileDir + "/" + FileSystem->getFileBasename(path.C_Str())));
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

            buffer->Vertices_Standard[j].Pos.X = vertex.x;
            buffer->Vertices_Standard[j].Pos.Y = vertex.y;
            buffer->Vertices_Standard[j].Pos.Z = vertex.z;

            //std::cout << "Coords=" << buffer->Vertices_Standard[j].Pos.X << ", " << buffer->Vertices_Standard[j].Pos.Y << ", " << buffer->Vertices_Standard[j].Pos.Z << std::endl;
            /*
            const aiVector3D* uv = paiMesh->mTextureCoords[0];
            if (uv != NULL)
            {
                buffer->Vertices_Standard[j].TCoords.X = uv[j].x;
                buffer->Vertices_Standard[j].TCoords.Y = uv[j].y;
            }
            */

            if (paiMesh->HasNormals())
            {
                aiVector3D normal = paiMesh->mNormals[j];
                buffer->Vertices_Standard[j].Normal = core::vector3df(normal.x, normal.y, normal.z);
            }

            if (paiMesh->HasVertexColors(0))
            {
                aiColor4D color = paiMesh->mColors[0][j] * 255.f;
                buffer->Vertices_Standard[j].Color = irr::video::SColor(color.a, color.r, color.g, color.b);
            }
            else
            {
                buffer->Vertices_Standard[j].Color = video::SColor(255, 255, 255, 255);
            }

            if (paiMesh->GetNumUVChannels() > 0)
            {
                const aiVector3D uv = paiMesh->mTextureCoords[0][j];
                buffer->Vertices_Standard[j].TCoords = core::vector2df(uv.x, uv.y);
            }
        }
        if (paiMesh->HasTangentsAndBitangents())
        {
            buffer->convertToTangents();
            for (unsigned int j = 0; j < paiMesh->mNumVertices; ++j)
            {
                aiVector3D tangent = paiMesh->mTangents[j];
                aiVector3D bitangent = paiMesh->mBitangents[j];
                buffer->Vertices_Tangents[j].Tangent = core::vector3df(tangent.x, tangent.y, tangent.z);
                buffer->Vertices_Tangents[j].Binormal = core::vector3df(bitangent.x, bitangent.y, bitangent.z);
            }
        }
        if (paiMesh->GetNumUVChannels() > 1)
        {
            buffer->convertTo2TCoords();
            for (unsigned int j = 0; j < paiMesh->mNumVertices; ++j)
            {
                const aiVector3D uv = paiMesh->mTextureCoords[0][j];
                buffer->Vertices_2TCoords[j].TCoords2 = core::vector2df(uv.x, uv.y);
            }
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

            //std::cout << "Name=" << bone->mName.C_Str() << std::endl;
            scene::ISkinnedMesh::SJoint* joint = findJoint(mesh, core::stringc(bone->mName.C_Str()));
            if (joint == 0)
                std::cout << "Error, no joint" << std::endl;

            //core::matrix4 globalBoneTransform = AssimpToIrrMatrix(getAbsoluteMatrixOf(findNode(pScene, bone->mName)));
            //core::matrix4 invRoot = AssimpToIrrMatrix(pScene->mRootNode->mTransformation.Inverse());

            core::matrix4 boneOffset = AssimpToIrrMatrix(bone->mOffsetMatrix);
            core::matrix4 invBoneOffset;
            boneOffset.getInverse(invBoneOffset);

            const core::vector3df nullVector = core::vector3df(0, 0, 0);

            const core::vector3df translation = boneOffset.getTranslation();
            core::vector3df translation2 = translation;
            //boneOffset.setInverseTranslation(translation);
            //invBoneOffset.rotateVect(translation2);

            core::matrix4 rotationMatrix;
            rotationMatrix.setRotationDegrees(invBoneOffset.getRotationDegrees());

            core::matrix4 translationMatrix;
            translationMatrix.setTranslation(translation);

            core::matrix4 globalBoneMatrix = rotationMatrix * translationMatrix;
            globalBoneMatrix.setInverseTranslation(globalBoneMatrix.getTranslation());
            //globalBoneMatrix.setRotationDegrees(nullVector);

            joint->GlobalMatrix = globalBoneMatrix;
            joint->LocalMatrix = globalBoneMatrix;

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
    for (u32 i = 0; i < mesh->getJointCount(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        computeLocal(mesh, pScene, joint);
    }

    int frameOffset = 0;

    for (unsigned int i = 0; i < pScene->mNumAnimations; ++i)
    {
        aiAnimation* anim = pScene->mAnimations[i];

        if (anim->mTicksPerSecond != 0.f)
        {
            mesh->setAnimationSpeed(anim->mTicksPerSecond);
        }

        //std::cout << "numChannels : " << anim->mNumChannels << std::endl;
        for (unsigned int j = 0; j < anim->mNumChannels; ++j)
        {
            aiNodeAnim* nodeAnim = anim->mChannels[j];
            scene::ISkinnedMesh::SJoint* joint = findJoint(mesh, nodeAnim->mNodeName.C_Str());

            for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mPositionKeys[k];

                scene::ISkinnedMesh::SPositionKey* irrKey = mesh->addPositionKey(joint);

                irrKey->frame = key.mTime + frameOffset;
                irrKey->position = core::vector3df(key.mValue.x, key.mValue.y, key.mValue.z);
            }
            for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; ++k)
            {
                aiQuatKey key = nodeAnim->mRotationKeys[k];
                aiQuaternion assimpQuat = key.mValue;

                core::quaternion quat (-assimpQuat.x, -assimpQuat.y, -assimpQuat.z, assimpQuat.w);

                scene::ISkinnedMesh::SRotationKey* irrKey = mesh->addRotationKey(joint);

                irrKey->frame = key.mTime + frameOffset;
                irrKey->rotation = quat;
            }
            for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mScalingKeys[k];

                scene::ISkinnedMesh::SScaleKey* irrKey = mesh->addScaleKey(joint);

                irrKey->frame = key.mTime + frameOffset;
                irrKey->scale = core::vector3df(key.mValue.x, key.mValue.y, key.mValue.z);
            }

        }

        frameOffset += anim->mDuration;
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

        irr::core::matrix4 globalParent = jointParent->GlobalMatrix;
        irr::core::matrix4 invGlobalParent;
        globalParent.getInverse(invGlobalParent);

        joint->LocalMatrix = invGlobalParent * joint->GlobalMatrix;
    }
    else
        ;   //std::cout << "Root ?" << std::endl;
}

bool IrrAssimp::isLoadable(irr::core::stringc path)
{
    Assimp::Importer Importer;

    irr::core::stringc extension;
    irr::core::getFileNameExtension(extension, path);
    return Importer.IsExtensionSupported (extension.c_str());
}
