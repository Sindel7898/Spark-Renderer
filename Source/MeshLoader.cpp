#include "MeshLoader.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image_write.h"

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

MeshLoader::MeshLoader()
{
}

void MeshLoader::LoadModel(const std::string& pFile)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, pFile);
    if (!warn.empty()) std::cout << "Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "Error: " << err << std::endl;
    if (!ret) {
        std::cerr << "Failed to load glTF" << std::endl;
        return;
    }

    std::cout << "Loaded glTF: " << pFile << std::endl;

    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            ProcessMesh(primitive, model);
        }
    }
}

void MeshLoader::ProcessMesh(const tinygltf::Primitive& primitive, tinygltf::Model& model)
{
    auto getBufferData = [&](const tinygltf::Accessor& accessor) -> const unsigned char* {
        const auto& view = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];
        return buffer.data.data() + view.byteOffset + accessor.byteOffset;
        };
    
    size_t vertexCount;
    const float* positions = nullptr;
    const float* normals = nullptr;
    const float* texcoords = nullptr;
    const float* tangents = nullptr;
    const float* bitangents = nullptr;

    // POSITION (required)
    if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
        const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
        positions = reinterpret_cast<const float*>(getBufferData(accessor));
    
        vertexCount = accessor.count;
        vertices.reserve(vertexCount);
    }
    else {
        std::cerr << "Missing POSITION attribute in mesh primitive." << std::endl;
        return;
    }

    // NORMAL
    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
        const auto& accessor = model.accessors[primitive.attributes.at("NORMAL")];
        normals = reinterpret_cast<const float*>(getBufferData(accessor));
    }
    else {
        std::cerr << "Missing NORMAL attribute in mesh primitive." << std::endl;

    }

    // TEXCOORD_0
    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
        const auto& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
        texcoords = reinterpret_cast<const float*>(getBufferData(accessor));
    }
    else {
        std::cerr << "Missing TEXCOORD_0 attribute in mesh primitive." << std::endl;

    }

    // TANGENT
    if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
        const auto& accessor = model.accessors[primitive.attributes.at("TANGENT")];
        tangents = reinterpret_cast<const float*>(getBufferData(accessor));
    }
    else {
        std::cerr << "Missing TANGENT attribute in mesh primitive." << std::endl;

    }

    // BITANGENT
    if (primitive.attributes.find("BITANGENT") != primitive.attributes.end()) {
        const auto& accessor = model.accessors[primitive.attributes.at("BITANGENT")];
        bitangents = reinterpret_cast<const float*>(getBufferData(accessor));
    }
    else {
        std::cerr << "Missing BITANGENT attribute in mesh primitive." << std::endl;

    }


    for (size_t i = 0; i < vertexCount; ++i) {
        ModelVertex vertex;
        vertex.vert = { positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] };

        if (normals) {
            vertex.normal = { normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] };
        }
        if (texcoords) {
            vertex.text = { texcoords[i * 2 + 0], texcoords[i * 2 + 1] };
        }
        if (tangents) {
            vertex.tangent = { tangents[i * 3 + 0], tangents[i * 3 + 1], tangents[i * 3 + 2] };
        }
        if (bitangents) {
            vertex.bitangent = { bitangents[i * 3 + 0], bitangents[i * 3 + 1], bitangents[i * 3 + 2] };
        }

        vertices.push_back(vertex);
    }

    const auto& indexAccessor = model.accessors[primitive.indices];
    const unsigned char* indexData = getBufferData(indexAccessor);
    uint32_t indexOffset = static_cast<uint32_t>(indices.size());
    indices.reserve(indices.size() + indexAccessor.count);

    switch (indexAccessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        for (size_t i = 0; i < indexAccessor.count; ++i)
            indices.push_back(indexOffset + reinterpret_cast<const uint16_t*>(indexData)[i]);
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        for (size_t i = 0; i < indexAccessor.count; ++i)
            indices.push_back(indexOffset + static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(indexData)[i]));
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        for (size_t i = 0; i < indexAccessor.count; ++i)
            indices.push_back(indexOffset + reinterpret_cast<const uint8_t*>(indexData)[i]);
        break;
    default:
        std::cerr << "Unsupported index component type" << std::endl;
        break;
    }

}

//void MeshLoader::ProcessNode(aiNode* node, const aiScene* scene)
//{
//    // Process each mesh located at this node
//    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
//        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
//        ProcessMesh(mesh, scene);
//    }
//
//    // After processing all meshes, process the children nodes recursively
//    for (unsigned int i = 0; i < node->mNumChildren; i++) {
//        this->ProcessNode(node->mChildren[i], scene);
//    }
//}
//
//void MeshLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
//{
//    // Process vertices
//    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
//
//        ModelVertex vertex;
//
//        if (mesh->HasPositions())
//        {
//            vertex.vert.x = mesh->mVertices[i].x;
//            vertex.vert.y = mesh->mVertices[i].y;
//            vertex.vert.z = mesh->mVertices[i].z;
//        }
//
//        if (mesh->HasTextureCoords(0))
//        {
//            vertex.text.x = (float)mesh->mTextureCoords[0][i].x;
//            vertex.text.y = (float)mesh->mTextureCoords[0][i].y;
//        }
//        else {
//            vertex.text.x = 0.0f;
//            vertex.text.y = 0.0f;
//        }
//
//        if (mesh->HasNormals()) {
//            vertex.normal.x = mesh->mNormals[i].x;
//            vertex.normal.y = mesh->mNormals[i].y;
//            vertex.normal.z = mesh->mNormals[i].z;
//        }
//
//        if (mesh->HasTangentsAndBitangents())
//        {
//            vertex.tangent.x = mesh->mTangents[i].x;
//            vertex.tangent.y = mesh->mTangents[i].y;
//            vertex.tangent.z = mesh->mTangents[i].z;
//
//            vertex.bitangent.x = mesh->mBitangents[i].x;
//            vertex.bitangent.y = mesh->mBitangents[i].y;
//            vertex.bitangent.z = mesh->mBitangents[i].z;
//        }
//
//        vertices.push_back(vertex);
//    }
//
//    // Process indices
//    for (uint16_t i = 0; i < mesh->mNumFaces; i++) {
//
//        aiFace face = mesh->mFaces[i];
//
//        for (uint16_t  j = 0; j < face.mNumIndices; j++) {
//
//            indices.push_back(face.mIndices[j]);
//
//        }
//    }
//}

const std::vector<ModelVertex>& MeshLoader::GetVertices() {
    return vertices;
}

const std::vector<uint16_t>& MeshLoader::GetIndices() {
    return indices;
}