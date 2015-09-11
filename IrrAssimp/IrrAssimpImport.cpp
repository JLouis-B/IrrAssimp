#include "IrrAssimpImport.h"
#include <iostream>

using namespace irr;

IrrAssimpImport::IrrAssimpImport(irr::scene::ISceneManager* smgr) : Smgr(smgr), FileSystem(smgr->getFileSystem())
{
    //ctor
}

IrrAssimpImport::~IrrAssimpImport()
{
    //dtor
}

void Log(core::vector3df vect)
{
    std::cout << "Vector = " << vect.X << ", " << vect.Y << ", " << vect.Z << std::endl;
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


scene::ISkinnedMesh::SJoint* IrrAssimpImport::findJoint (scene::ISkinnedMesh* mesh, core::stringc jointName)
{
    for (unsigned int i = 0; i < mesh->getJointCount(); ++i)
    {
        if (core::stringc(mesh->getJointName(i)) == jointName)
            return mesh->getAllJoints()[i];
    }
    std::cout << "Error, no joint" << std::endl;
    return 0;
}

aiNode* IrrAssimpImport::findNode (const aiScene* scene, aiString jointName)
{
    if (scene->mRootNode->mName == jointName)
        return scene->mRootNode;

    return scene->mRootNode->FindNode(jointName);
}

void IrrAssimpImport::createNode(scene::ISkinnedMesh* mesh, aiNode* node, bool isRoot)
{
    scene::ISkinnedMesh::SJoint* jointParent = 0;

    if (node->mParent != 0)
    {
        jointParent = findJoint(mesh, node->mParent->mName.C_Str());
    }

    scene::ISkinnedMesh::SJoint* joint = mesh->addJoint(jointParent);
    joint->Name = node->mName.C_Str();
    joint->LocalMatrix = AssimpToIrrMatrix(node->mTransformation);


    if (jointParent)
        joint->GlobalMatrix = jointParent->GlobalMatrix * joint->LocalMatrix;
    else
        joint->GlobalMatrix = joint->LocalMatrix;


    if (isRoot)
        joint->GlobalMatrix.getInverse(InverseRootNodeWorldTransform);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        joint->AttachedMeshes.push_back(node->mMeshes[i]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        aiNode* childNode = node->mChildren[i];
        createNode(mesh, childNode, false);
    }
}

video::SColor AssimpToIrrColor(aiColor4D color)
{
    return video::SColor(color.a * 255.f, color.r * 255.f, color.g * 255.f, color.b * 255.f);
}

video::ITexture* IrrAssimpImport::getTexture(core::stringc path, core::stringc fileDir)
{
    video::ITexture* texture = 0;

    if (FileSystem->existFile(path.c_str()))
        texture = Smgr->getVideoDriver()->getTexture(path.c_str());
    else if (FileSystem->existFile(fileDir + "/" + path.c_str()))
        texture = Smgr->getVideoDriver()->getTexture(fileDir + "/" + path.c_str());
    else if (FileSystem->existFile(fileDir + "/" + FileSystem->getFileBasename(path.c_str())))
        texture = Smgr->getVideoDriver()->getTexture(fileDir + "/" + FileSystem->getFileBasename(path.c_str()));

    return texture;
    // TODO after 1.9 release : Rewrite this with IMeshTextureLoader
}


bool IrrAssimpImport::isALoadableFileExtension(const io::path& filename) const
{
    Assimp::Importer importer;

    io::path extension;
    irr::core::getFileNameExtension(extension, filename);
    return importer.IsExtensionSupported (to_char_string(extension).c_str());
}

irr::scene::IAnimatedMesh* IrrAssimpImport::createMesh(irr::io::IReadFile* file)
{
    irr::io::path path = file->getFileName();

    Assimp::Importer Importer;
    const aiScene* pScene = Importer.ReadFile(to_char_string(path).c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

    if (!pScene)
    {
        Error = Importer.GetErrorString();
        return 0;
    }
    else
        Error = "";

    core::stringc fileDir = FileSystem->getFileDir(path);

    Mats.clear();

    // Create mesh
    scene::ISkinnedMesh* mesh = Smgr->createSkinnedMesh();

    // Basic material support
    for (unsigned int i = 0; i < pScene->mNumMaterials; ++i)
    {
        video::SMaterial material;
        material.MaterialType = video::EMT_SOLID;

        aiMaterial* mat = pScene->mMaterials[i];

        aiColor4D color;
        if(AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color)) {
            material.DiffuseColor = AssimpToIrrColor(color);
        }
        if(AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &color)) {
            material.AmbientColor = AssimpToIrrColor(color);
        }
        if(AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &color)) {
            material.EmissiveColor = AssimpToIrrColor(color);
        }
        if(AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &color)) {
            material.SpecularColor = AssimpToIrrColor(color);
        }
        float shininess;
        if(AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &shininess)) {
            material.Shininess = shininess;
        }


        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString path;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);

            video::ITexture* diffuseTexture = getTexture(path.C_Str(), fileDir);
            material.setTexture(0, diffuseTexture);
        }
        if (mat->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            aiString path;
            mat->GetTexture(aiTextureType_NORMALS, 0, &path);

            video::ITexture* normalsTexture = getTexture(path.C_Str(), fileDir);
            if (normalsTexture)
            {
                material.setTexture(1, normalsTexture);
                material.MaterialType = video::EMT_PARALLAX_MAP_SOLID;
            }
        }

        Mats.push_back(material);
    }

    aiNode* root = pScene->mRootNode;
    createNode(mesh, root, true);

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
            buffer->Vertices_Standard[j].Pos = core::vector3df(vertex.x, vertex.y, vertex.z);

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

        buffer->Material = Mats[paiMesh->mMaterialIndex];
        buffer->recalculateBoundingBox();

        if (!paiMesh->HasNormals())
            Smgr->getMeshManipulator()->recalculateNormals(buffer);

        buildSkinnedVertexArray(buffer);
        for (unsigned int j = 0; j < paiMesh->mNumBones; ++j)
        {
            aiBone* bone = paiMesh->mBones[j];

            scene::ISkinnedMesh::SJoint* joint = findJoint(mesh, core::stringc(bone->mName.C_Str()));
            if (joint == 0)
            {
                std::cout << "Error, no joint" << std::endl;
                continue;
            }



            for (unsigned int h = 0; h < bone->mNumWeights; ++h)
            {
                aiVertexWeight weight = bone->mWeights[h];
                scene::ISkinnedMesh::SWeight* w = mesh->addWeight(joint);
                w->buffer_id = mesh->getMeshBufferCount() - 1;
                w->strength = weight.mWeight;
                w->vertex_id = weight.mVertexId;
            }

            skinJoint(mesh, joint, bone);
        }
        applySkinnedVertexArray(buffer);
    }

    int frameOffset = 0;
    for (unsigned int i = 0; i < pScene->mNumAnimations; ++i)
    {
        aiAnimation* anim = pScene->mAnimations[i];

        if (anim->mTicksPerSecond != 0.f)
        {
            mesh->setAnimationSpeed(anim->mTicksPerSecond);
        }
		// Some loader of assimp give time in second for keyframe instead of frame number, which cause bug when casted to int
        if (anim->mTicksPerSecond == 1)
            mesh->setAnimationSpeed(mesh->getAnimationSpeed() * 60.f);

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
                if (anim->mTicksPerSecond == 1)
                    irrKey->frame *= 60.f;
                irrKey->position = core::vector3df(key.mValue.x, key.mValue.y, key.mValue.z);
            }
            for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; ++k)
            {
                aiQuatKey key = nodeAnim->mRotationKeys[k];
                aiQuaternion assimpQuat = key.mValue;

                core::quaternion quat (-assimpQuat.x, -assimpQuat.y, -assimpQuat.z, assimpQuat.w);
				quat.normalize();

                scene::ISkinnedMesh::SRotationKey* irrKey = mesh->addRotationKey(joint);

                irrKey->frame = key.mTime + frameOffset;
                if (anim->mTicksPerSecond == 1)
                    irrKey->frame *= 60.f;
                irrKey->rotation = quat;
            }
            for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mScalingKeys[k];

                scene::ISkinnedMesh::SScaleKey* irrKey = mesh->addScaleKey(joint);

                irrKey->frame = key.mTime + frameOffset;
                if (anim->mTicksPerSecond == 1)
                    irrKey->frame *= 60.f;
                irrKey->scale = core::vector3df(key.mValue.x, key.mValue.y, key.mValue.z);
            }

        }

        frameOffset += anim->mDuration;
    }

    mesh->setDirty();
    mesh->finalize();

    return mesh;
}

// Adapted from http://sourceforge.net/p/assimp/discussion/817654/thread/5462cbf5
void IrrAssimpImport::skinJoint(scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint *joint, aiBone* bone)
{
	if (bone->mNumWeights)
	{
	    irr::core::matrix4 boneOffset = AssimpToIrrMatrix(bone->mOffsetMatrix);
	    irr::core::matrix4 boneMat = joint->GlobalMatrix * boneOffset; //* InverseRootNodeWorldTransform;

        const u32 bufferId = mesh->getMeshBufferCount() - 1;

		for (u32 i = 0; i < bone->mNumWeights; ++i)
		{
		    aiVertexWeight weight = bone->mWeights[i];
		    /*
		    scene::ISkinnedMesh::SWeight weight = joint->Weights[i];

		    if (weight.buffer_id != mesh->getMeshBufferCount() - 1)
                continue;*/

			const u32 vertexId = weight.mVertexId;
			core::vector3df sourcePos = mesh->getMeshBuffer(bufferId)->getPosition(vertexId);
			core::vector3df sourceNorm = mesh->getMeshBuffer(bufferId)->getNormal(vertexId);
			core::vector3df destPos, destNormal;
			boneMat.transformVect(destPos, sourcePos);
			boneMat.rotateVect(destNormal, sourceNorm);

			skinnedVertex[vertexId].Moved = true;
            skinnedVertex[vertexId].Position += destPos * weight.mWeight;
            skinnedVertex[vertexId].Normal += destNormal * weight.mWeight;
		}
	}
}

void IrrAssimpImport::buildSkinnedVertexArray(scene::IMeshBuffer* buffer)
{
    skinnedVertex.clear();

    skinnedVertex.reallocate(buffer->getVertexCount());
    skinnedVertex.set_used(buffer->getVertexCount());

    for (u32 i = 0; i < buffer->getVertexCount(); ++i)
    {
        skinnedVertex[i] = SkinnedVertex();
    }

}

void IrrAssimpImport::applySkinnedVertexArray(scene::IMeshBuffer* buffer)
{
    for (u32 i = 0; i < buffer->getVertexCount(); ++i)
    {
        if (skinnedVertex[i].Moved)
        {
            buffer->getPosition(i) = skinnedVertex[i].Position;
            buffer->getNormal(i) = skinnedVertex[i].Normal;
        }
    }
    skinnedVertex.clear();
}
