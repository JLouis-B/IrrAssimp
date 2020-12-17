#include "IrrAssimpImport.h"

#include <ISceneManager.h>
#include <IVideoDriver.h>
#include <IMeshManipulator.h>

using namespace irr;

IrrAssimpImport::IrrAssimpImport(scene::ISceneManager* smgr) : Smgr(smgr), FileSystem(smgr->getFileSystem())
{
    //ctor
}

IrrAssimpImport::~IrrAssimpImport()
{
    //dtor
}

core::stringc assimpToIrrString(aiString str)
{
    return core::stringc(str.C_Str());
}

core::vector3df assimpToIrrVector3(const aiVector3D& vect)
{
    return core::vector3df(vect.x, vect.y, vect.z);
}

core::vector2df assimpToIrrVector2(const aiVector3D& vect)
{
    return core::vector2df(vect.x, vect.y);
}

core::quaternion assimpToIrrQuaternion(const aiQuaternion& quat)
{
    return core::quaternion(quat.x, quat.y, quat.z, -quat.w);
}

core::matrix4 assimpToIrrMatrix(aiMatrix4x4 assimpMatrix)
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

video::SColor assimpToIrrColor4(const aiColor4D& color)
{
    return video::SColor(static_cast<u32>(color.a * 255), static_cast<u32>(color.r * 255), static_cast<u32>(color.g * 255), static_cast<u32>(color.b * 255));
}


scene::ISkinnedMesh::SJoint* IrrAssimpImport::findJoint(const core::stringc jointName)
{
    for (unsigned int i = 0; i < Mesh->getJointCount(); ++i)
    {
        if (core::stringc(Mesh->getJointName(i)) == jointName)
            return Mesh->getAllJoints()[i];
    }
    //std::cout << "Error, no joint" << std::endl;
    return 0;
}

aiNode* IrrAssimpImport::findNode(const aiString jointName)
{
    if (AssimpScene->mRootNode->mName == jointName)
        return AssimpScene->mRootNode;

    return AssimpScene->mRootNode->FindNode(jointName);
}

void IrrAssimpImport::createNode(const aiNode* node)
{
    scene::ISkinnedMesh::SJoint* jointParent = 0;

    if (node->mParent != 0)
    {
        jointParent = findJoint(assimpToIrrString(node->mParent->mName));
    }

    scene::ISkinnedMesh::SJoint* joint = Mesh->addJoint(jointParent);
    joint->Name = assimpToIrrString(node->mName);
    joint->LocalMatrix = assimpToIrrMatrix(node->mTransformation);

    if (jointParent)
        joint->GlobalMatrix = jointParent->GlobalMatrix * joint->LocalMatrix;
    else
        joint->GlobalMatrix = joint->LocalMatrix;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        joint->AttachedMeshes.push_back(node->mMeshes[i]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        aiNode* childNode = node->mChildren[i];
        createNode(childNode);
    }
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
    core::getFileNameExtension(extension, filename);
    return importer.IsExtensionSupported (extension.c_str());
}

scene::IAnimatedMesh* IrrAssimpImport::createMesh(io::IReadFile* file)
{
    FilePath = file->getFileName();

    Assimp::Importer Importer;
    AssimpScene = Importer.ReadFile(irrToAssimpPath(FilePath).C_Str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
    if (!AssimpScene)
    {
        Error = Importer.GetErrorString();
        return 0;
    }
    else
        Error = "";

    // Create mesh
    Mesh = Smgr->createSkinnedMesh();

    // Load materials
    createMaterials();

    // Load nodes
    const aiNode* root = AssimpScene->mRootNode;
    createNode(root);

    // Load meshes
    createMeshes();

    // Load animations
    createAnimation();

    Mesh->setDirty();
    Mesh->finalize();

    return Mesh;
}

void IrrAssimpImport::createMaterials()
{
    // Basic material support
    const core::stringc fileDir = FileSystem->getFileDir(FilePath);
    Mats.clear();
    for (unsigned int i = 0; i < AssimpScene->mNumMaterials; ++i)
    {
        video::SMaterial material;
        material.MaterialType = video::EMT_SOLID;

        aiMaterial* mat = AssimpScene->mMaterials[i];

        aiColor4D color;
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color)) {
            material.DiffuseColor = assimpToIrrColor4(color);
        }
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &color)) {
            material.AmbientColor = assimpToIrrColor4(color);
        }
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &color)) {
            material.EmissiveColor = assimpToIrrColor4(color);
        }
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &color)) {
            material.SpecularColor = assimpToIrrColor4(color);
        }
        float shininess;
        if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &shininess)) {
            material.Shininess = shininess;
        }


        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString path;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);

            video::ITexture* diffuseTexture = getTexture(assimpToIrrString(path), fileDir);
            material.setTexture(0, diffuseTexture);
        }
        if (mat->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            aiString path;
            mat->GetTexture(aiTextureType_NORMALS, 0, &path);

            video::ITexture* normalsTexture = getTexture(assimpToIrrString(path), fileDir);
            if (normalsTexture)
            {
                material.setTexture(1, normalsTexture);
                material.MaterialType = video::EMT_PARALLAX_MAP_SOLID;
            }
        }

        Mats.push_back(material);
    }
}

void IrrAssimpImport::createMeshes()
{
    for (unsigned int i = 0; i < AssimpScene->mNumMeshes; ++i)
    {
        //std::cout << "i=" << i << " of " << AssimpScene->mNumMeshes << std::endl;
        aiMesh* paiMesh = AssimpScene->mMeshes[i];
        const unsigned int uvCount = paiMesh->GetNumUVChannels();
        const unsigned int verticesCount = paiMesh->mNumVertices;

        scene::SSkinMeshBuffer* buffer = Mesh->addMeshBuffer();
        buffer->VertexType = (uvCount > 1) ? video::EVT_2TCOORDS : (paiMesh->HasTangentsAndBitangents() ? video::EVT_TANGENTS : video::EVT_STANDARD);

        // Resize Buffer
        switch (buffer->VertexType)
        {
        case video::EVT_STANDARD:
            buffer->Vertices_Standard.set_used(verticesCount);
            break;
        case video::EVT_2TCOORDS:
            buffer->Vertices_2TCoords.set_used(verticesCount);
            break;
        case video::EVT_TANGENTS:
            buffer->Vertices_Tangents.set_used(verticesCount);
            break;
        }


        for (unsigned int j = 0; j < verticesCount; ++j)
        {
            buffer->getPosition(j) = assimpToIrrVector3(paiMesh->mVertices[j]);
            if (paiMesh->HasNormals())
            {
                buffer->getNormal(j) = assimpToIrrVector3(paiMesh->mNormals[j]);
            }

            video::SColor irrColor = video::SColor(255, 255, 255, 255);
            if (paiMesh->HasVertexColors(0))
            {
                irrColor = assimpToIrrColor4(paiMesh->mColors[0][j]);
            }

            core::vector2df irrUv = core::vector2df(0.f, 0.f);
            if (uvCount > 0)
            {
                irrUv = assimpToIrrVector2(paiMesh->mTextureCoords[0][j]);
            }
            buffer->getTCoords(j) = irrUv;

            switch (buffer->VertexType)
            {
                case video::EVT_STANDARD:
                {
                    buffer->Vertices_Standard[j].Color = irrColor;
                    break;
                }
                case video::EVT_2TCOORDS:
                {
                    buffer->Vertices_2TCoords[j].Color = irrColor;
                    buffer->Vertices_2TCoords[j].TCoords2 = assimpToIrrVector2(paiMesh->mTextureCoords[1][j]);
                    break;
                }
                case video::EVT_TANGENTS:
                {
                    buffer->Vertices_Tangents[j].Color = irrColor;
                    buffer->Vertices_Tangents[j].Tangent = assimpToIrrVector3(paiMesh->mTangents[j]);
                    buffer->Vertices_Tangents[j].Binormal = assimpToIrrVector3(paiMesh->mBitangents[j]);
                    break;
                }
            }
        }


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


        // Skinning
        buildSkinnedVertexArray(buffer);
        for (unsigned int j = 0; j < paiMesh->mNumBones; ++j)
        {
            aiBone* bone = paiMesh->mBones[j];

            scene::ISkinnedMesh::SJoint* joint = findJoint(assimpToIrrString(bone->mName));
            if (joint == 0)
            {
                //std::cout << "Error, no joint" << std::endl;
                continue;
            }

            /* The old version
            core::matrix4 boneOffset = AssimpToIrrMatrix(bone->mOffsetMatrix);
            core::matrix4 invBoneOffset;
            boneOffset.getInverse(invBoneOffset);

            core::matrix4 rotationMatrix;
            rotationMatrix.setRotationDegrees(invBoneOffset.getRotationDegrees());
            core::matrix4 translationMatrix;
            translationMatrix.setTranslation(boneOffset.getTranslation());
			core::matrix4 scaleMatrix;
            scaleMatrix.setScale(invBoneOffset.getScale());

            core::matrix4 globalBoneMatrix = scaleMatrix * rotationMatrix * translationMatrix;
            globalBoneMatrix.setInverseTranslation(globalBoneMatrix.getTranslation());

            joint->GlobalMatrix = globalBoneMatrix;
            // And compute the local from global after (joint->LocalMatrix = invGlobalParent * joint->GlobalMatrix; if it's not a root)
            */


            for (unsigned int h = 0; h < bone->mNumWeights; ++h)
            {
                const aiVertexWeight weight = bone->mWeights[h];
                scene::ISkinnedMesh::SWeight* irrWeight = Mesh->addWeight(joint);
                irrWeight->buffer_id = Mesh->getMeshBufferCount() - 1;
                irrWeight->strength = weight.mWeight;
                irrWeight->vertex_id = weight.mVertexId;
            }

            skinJoint(joint, bone);
        }
        applySkinnedVertexArray(buffer);
    }
}

void IrrAssimpImport::createAnimation()
{
    double frameOffset = 0.f;
    for (unsigned int i = 0; i < AssimpScene->mNumAnimations; ++i)
    {
        aiAnimation* anim = AssimpScene->mAnimations[i];

        if (anim->mTicksPerSecond != 0.f)
        {
            Mesh->setAnimationSpeed(anim->mTicksPerSecond);
        }

        //std::cout << "numChannels : " << anim->mNumChannels << std::endl;
        for (unsigned int j = 0; j < anim->mNumChannels; ++j)
        {
            aiNodeAnim* nodeAnim = anim->mChannels[j];
            scene::ISkinnedMesh::SJoint* joint = findJoint(assimpToIrrString(nodeAnim->mNodeName));

            for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mPositionKeys[k];

                scene::ISkinnedMesh::SPositionKey* irrKey = Mesh->addPositionKey(joint);
                irrKey->frame = key.mTime + frameOffset;
                irrKey->position = assimpToIrrVector3(key.mValue);
            }
            for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; ++k)
            {
                aiQuatKey key = nodeAnim->mRotationKeys[k];

                scene::ISkinnedMesh::SRotationKey* irrKey = Mesh->addRotationKey(joint);
                irrKey->frame = key.mTime + frameOffset;
                irrKey->rotation = assimpToIrrQuaternion(key.mValue);
            }
            for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mScalingKeys[k];

                scene::ISkinnedMesh::SScaleKey* irrKey = Mesh->addScaleKey(joint);
                irrKey->frame = key.mTime + frameOffset;
                irrKey->scale = assimpToIrrVector3(key.mValue);
            }

        }

        frameOffset += anim->mDuration;
    }
}

// Adapted from http://sourceforge.net/p/assimp/discussion/817654/thread/5462cbf5
void IrrAssimpImport::skinJoint(scene::ISkinnedMesh::SJoint* joint, aiBone* bone)
{
	if (bone->mNumWeights)
	{
        core::matrix4 boneOffset = assimpToIrrMatrix(bone->mOffsetMatrix);
	    core::matrix4 boneMat = joint->GlobalMatrix * boneOffset; //* InverseRootNodeWorldTransform;

        const u32 bufferId = Mesh->getMeshBufferCount() - 1;

		for (u32 i = 0; i < bone->mNumWeights; ++i)
		{
		    const aiVertexWeight weight = bone->mWeights[i];
			const u32 vertexId = weight.mVertexId;

			core::vector3df sourcePos = Mesh->getMeshBuffer(bufferId)->getPosition(vertexId);
			core::vector3df sourceNorm = Mesh->getMeshBuffer(bufferId)->getNormal(vertexId);
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
    for (u32 i = 0; i < buffer->getVertexCount(); ++i)
    {
        skinnedVertex.push_back(SkinnedVertex());
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
