#include "MeshLoader.h"
#include <iostream>

MeshLoader::MeshLoader()
{
}

void MeshLoader::LoadModel(const std::string& pFile)
{
    const aiScene* scene = importer.ReadFile(pFile, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
        return;
    }

    // Process the root node to start loading the model's meshes
    if (scene) {

        ProcessNode(scene->mRootNode, scene);

    }
}

void MeshLoader::ProcessNode(aiNode* node, const aiScene* scene)
{
    // Process each mesh located at this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene);
    }

    // After processing all meshes, process the children nodes recursively
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        this->ProcessNode(node->mChildren[i], scene);
    }
}

void MeshLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {

        ModelVertex vertex;

        vertex.vert.x = mesh->mVertices[i].x;
        vertex.vert.y = mesh->mVertices[i].y;
        vertex.vert.z = mesh->mVertices[i].z;



        /*  if (mesh->HasNormals())
          {
              vertex.norm.x = mesh->mNormals[i].x;
              vertex.norm.y = mesh->mNormals[i].y;
              vertex.norm.z = mesh->mNormals[i].z;
          }*/

        if (mesh->HasTextureCoords(0))
        {
            vertex.text.x = (float)mesh->mTextureCoords[0][i].x;
            vertex.text.y = (float)mesh->mTextureCoords[0][i].y;
        }
        else {
            vertex.text.x = 0.0f;
            vertex.text.y = 0.0f;
        }

        /*if (mesh->HasTangentsAndBitangents())
        {
            vertex.tangent.x = mesh->mTangents[i].x;
            vertex.tangent.y = mesh->mTangents[i].y;
            vertex.tangent.z = mesh->mTangents[i].z;

            vertex.bitangent.x = mesh->mBitangents[i].x;
            vertex.bitangent.y = mesh->mBitangents[i].y;
            vertex.bitangent.z = mesh->mBitangents[i].z;
        }*/


        vertices.push_back(vertex);
    }

    // Process indices
    for (uint16_t i = 0; i < mesh->mNumFaces; i++) {

        aiFace face = mesh->mFaces[i];

        for (uint16_t  j = 0; j < face.mNumIndices; j++) {

            indices.push_back(face.mIndices[j]);

        }
    }
}

const std::vector<ModelVertex>& MeshLoader::GetVertices() {
    return vertices;
}

const std::vector<uint16_t>& MeshLoader::GetIndices() {
    return indices;
}