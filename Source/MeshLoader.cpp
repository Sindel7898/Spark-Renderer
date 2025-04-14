#include "MeshLoader.h"
#include <iostream>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"
#include "mikktspace.h"
#include "AssetManager.h"

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

    if (!model.textures.empty()) {

        std::vector<StoredImageData> Textures;

            StoredImageData TextureData;

       
            const tinygltf::Material& material = model.materials[0];
            int baseColorTextureindex = material.pbrMetallicRoughness.baseColorTexture.index;

            const tinygltf::Texture& colortex = model.textures[baseColorTextureindex];
            const tinygltf::Image& image = model.images[colortex.source];
           
            size_t colorimageSize = image.width * image.height * 4; 
            TextureData.imageData = new stbi_uc[colorimageSize];
            std::memcpy(TextureData.imageData, image.image.data(), colorimageSize);
            TextureData.imageHeight = image.height;
            TextureData.imageWidth = image.width;

            Textures.push_back(TextureData);
             /////////////////////////////////////////////////////////////////////

            int noramlmaterialIndex = material.normalTexture.index;
            const tinygltf::Texture& normaltex = model.textures[noramlmaterialIndex];
            const tinygltf::Image& noramlimage = model.images[normaltex.source];

            size_t noramlimagesize = noramlimage.width * noramlimage.height * 4;
            TextureData.imageData = new stbi_uc[noramlimagesize];
            std::memcpy(TextureData.imageData, noramlimage.image.data(), noramlimagesize);
            TextureData.imageHeight = noramlimage.height;
            TextureData.imageWidth = noramlimage.width;

            Textures.push_back(TextureData);

            
        AssetManager::GetInstance().ParseTextureData(pFile, Textures);
    }
    else {
        std::cout << "No textures found in the model.\n";
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

    for (size_t i = 0; i < vertexCount; ++i) {
        ModelVertex vertex;
        vertex.vert = { positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] };

        if (normals) {
            vertex.normal = { normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] };
        }
        if (texcoords) {
            vertex.text = { texcoords[i * 2 + 0], texcoords[i * 2 + 1] };
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

    // Generate tangents and bitangents using MikkTSpace
    MikkTSpaceUserData userData(vertices, indices);
    SMikkTSpaceInterface iface = {};
    iface.m_getNumFaces = GetNumFaces;
    iface.m_getNumVerticesOfFace = GetNumVerticesOfFace;
    iface.m_getPosition = GetPosition;
    iface.m_getNormal = GetNormal;
    iface.m_getTexCoord = GetTexCoord;
    iface.m_setTSpaceBasic = SetTangent;

    SMikkTSpaceContext context = {};
    context.m_pInterface = &iface;
    context.m_pUserData = &userData;

    genTangSpaceDefault(&context);


}


const std::vector<ModelVertex>& MeshLoader::GetVertices() {
    return vertices;
}

const std::vector<uint32_t >& MeshLoader::GetIndices() {
    return indices;
}


int GetNumFaces(const SMikkTSpaceContext* context) {
    auto* data = static_cast<MikkTSpaceUserData*>(context->m_pUserData);
    return static_cast<int>(data->indices.size()) / 3;
}

int GetNumVerticesOfFace(const SMikkTSpaceContext*, int) {
    return 3;
}

void GetPosition(const SMikkTSpaceContext* context, float pos[3], int faceIdx, int vertIdx) {
    auto* data = static_cast<MikkTSpaceUserData*>(context->m_pUserData);
    int idx = data->indices[faceIdx * 3 + vertIdx];
    ////////////////////////////////////////////////
    const auto& v = data->vertices[idx].vert;
    pos[0] = v.x; pos[1] = v.y; pos[2] = v.z;
}

void GetNormal(const SMikkTSpaceContext* context, float norm[3], int faceIdx, int vertIdx) {
    auto* data = static_cast<MikkTSpaceUserData*>(context->m_pUserData);
    int idx = data->indices[faceIdx * 3 + vertIdx];
    ////////////////////////////////////////////////
    const auto& n = data->vertices[idx].normal;
    norm[0] = n.x; norm[1] = n.y; norm[2] = n.z;
}

void GetTexCoord(const SMikkTSpaceContext* context, float uv[2], int faceIdx, int vertIdx) {
    auto* data = static_cast<MikkTSpaceUserData*>(context->m_pUserData);
    int idx = data->indices[faceIdx * 3 + vertIdx];
    ////////////////////////////////////////////////
    const auto& t = data->vertices[idx].text;
    uv[0] = t.x; uv[1] = t.y;
}

void SetTangent(const SMikkTSpaceContext* context, const float tangent[3], float sign, int faceIdx, int vertIdx) {
    auto* data = static_cast<MikkTSpaceUserData*>(context->m_pUserData);
    int idx = data->indices[faceIdx * 3 + vertIdx];

    data->vertices[idx].tangent = { tangent[0], tangent[1], tangent[2] };
    data->vertices[idx].bitangent = sign * glm::cross(data->vertices[idx].normal, data->vertices[idx].tangent);
}