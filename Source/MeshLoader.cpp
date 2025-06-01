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
#include <glm/gtc/type_ptr.hpp>

MeshLoader::MeshLoader()
{
}

void MeshLoader::LoadModel(const std::string& pFile)
{
    FilePath = pFile;
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
   
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, pFile);
    if (!warn.empty()) std::cout << "Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "Error: " << err << std::endl;
    if (!ret) {
        std::cerr << "Failed to load glTF" << std::endl;
        return;
    }

   
    LoadMaterials(pFile, model);

    std::cout << "Loaded glTF: " << pFile << std::endl;

    for (size_t i = 0; i < model.scenes.size(); i++)
    {
        const tinygltf::Scene& scene = model.scenes[i];

        for (size_t j = 0; j < scene.nodes.size(); j++) {

            const tinygltf::Node node = model.nodes[scene.nodes[j]];

            ProcessNode(node, model);
        }
    }

}

void MeshLoader::LoadMaterials(const std::string& pFile, tinygltf::Model& model)
{
    if (!model.textures.empty()) {

        //create the list of textures array
        std::vector<StoredImageData> Textures;

        // For all the "Materials group" get the individual material  
        for (int i = 0; i < model.materials.size(); i++)
        {
            tinygltf::Material gltfMaterial = model.materials[i];

            {
               //check if the material group has the albedo texture in its map
               if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end())
               {
                   StoredImageData TextureData;
                   //get the texture from the material map
                   tinygltf::Texture& colortex = model.textures[gltfMaterial.values["baseColorTexture"].TextureIndex()];
                   //get the image from the texture
                   const tinygltf::Image& image = model.images[colortex.source];
               
                   size_t colorimageSize = image.width * image.height * 4;
                   TextureData.imageData = new stbi_uc[colorimageSize];
                   std::memcpy(TextureData.imageData, image.image.data(), colorimageSize);
                   TextureData.imageHeight = image.height;
                   TextureData.imageWidth = image.width;
               
                   Textures.push_back(TextureData);
               }
            }

            {
               //check if the material group has the normal texture in its map. 
               if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end())
               {
                   StoredImageData TextureData;
                   //get the texture from the material map
                   tinygltf::Texture& normaltex = model.textures[gltfMaterial.additionalValues["normalTexture"].TextureIndex()];
                   //get the image from the texture
                   const tinygltf::Image& image = model.images[normaltex.source];
               
                   size_t normalimageSize = image.width * image.height * 4;
                   TextureData.imageData = new stbi_uc[normalimageSize];
                   std::memcpy(TextureData.imageData, image.image.data(), normalimageSize);
                   TextureData.imageHeight = image.height;
                   TextureData.imageWidth = image.width;
               
                   Textures.push_back(TextureData);
               }
            }

            {
                //check if the material group has the normal texture in its map. 
                if (gltfMaterial.additionalValues.find("MetalRoughnessAOPack") != gltfMaterial.additionalValues.end())
                {
                    StoredImageData TextureData;
                    //get the texture from the material map
                    tinygltf::Texture& metalicroughnesstex = model.textures[gltfMaterial.additionalValues["MetalRoughnessAOPack"].TextureIndex()];
                    //get the image from the texture
                    const tinygltf::Image& image = model.images[metalicroughnesstex.source];

                    size_t metalicroughnessimageSize = image.width * image.height * 4;
                    TextureData.imageData = new stbi_uc[metalicroughnessimageSize];
                    std::memcpy(TextureData.imageData, image.image.data(), metalicroughnessimageSize);
                    TextureData.imageHeight = image.height;
                    TextureData.imageWidth = image.width;

                    Textures.push_back(TextureData);
                }
            }
        }

        AssetManager::GetInstance().ParseTextureData(pFile, Textures);
    }
    else {
        std::cout << "No textures found in the model.\n";
    }
}

void MeshLoader::ProcessNode(const tinygltf::Node& node, const tinygltf::Model& model) {
    // Process the current node
    ProcessMesh(node, model);

    // Recursively process all child nodes
    for (int childIndex : node.children) {

        const tinygltf::Node& childNode = model.nodes[childIndex];

        ProcessNode(childNode, model);
    }
}


void MeshLoader::ProcessMesh(const tinygltf::Node& inputNode, const tinygltf::Model& model)
{
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t > indices;

    glm::mat4 ModelMatrix = glm::mat4(1.0f);


 

    if (inputNode.matrix.size() == 16)
    {
        ModelMatrix = glm::make_mat4x4(inputNode.matrix.data());
    }
    else
    {
        if (inputNode.translation.size() == 3)
        {
            ModelMatrix = glm::translate(ModelMatrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
        }

        if (inputNode.rotation.size() == 4)
        {
            glm::quat q = glm::make_quat(inputNode.rotation.data());
            ModelMatrix *= glm::mat4(q);
        }

        if (inputNode.scale.size() == 3)
        {
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
        }
    }


    if (inputNode.mesh > -1)
    {
        const tinygltf::Mesh mesh = model.meshes[inputNode.mesh];

        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            uint32_t indexCount = 0;

            const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
            
            const float* positions = nullptr;
            const float* normals = nullptr;
            const float* texcoords = nullptr;
            size_t vertexCount;


            if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView& view   = model.bufferViews[accessor.bufferView];
                positions = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                vertexCount = accessor.count;
            }

            if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& view   = model.bufferViews[accessor.bufferView];
                normals = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }

            if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView& view   = model.bufferViews[accessor.bufferView];
                texcoords = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }

            for (size_t i = 0; i < vertexCount; ++i) {
                ModelVertex vertex;

                if (positions)
                {
                    vertex.vert = { positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] };
                }
                if (normals) {
                    vertex.normal = { normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] };
                }
                if (texcoords) {
                    vertex.text = { texcoords[i * 2 + 0], texcoords[i * 2 + 1] };
                }

                vertices.push_back(vertex);
            }

            const auto& indexAccessor = model.accessors[glTFPrimitive.indices];
            const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            indexCount += static_cast<uint32_t>(indexAccessor.count);

            switch (indexAccessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[indexAccessor.byteOffset + bufferView.byteOffset]);
                for (size_t index = 0; index < indexAccessor.count; index++) {
                    indices.push_back(buf[index]);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[indexAccessor.byteOffset + bufferView.byteOffset]);
                for (size_t index = 0; index < indexAccessor.count; index++) {
                    indices.push_back(buf[index]);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[indexAccessor.byteOffset + bufferView.byteOffset]);
                for (size_t index = 0; index < indexAccessor.count; index++) {
                    indices.push_back(buf[index]);
                }
                break;
            }
            default:
                std::cerr << "Index component type " << indexAccessor.componentType << " not supported!" << std::endl;
                return;
            }


        }

        StoredModelData modeldata; 
        modeldata.VertexData  = vertices;
        modeldata.IndexData   = indices;
        modeldata.modelMatrix = ModelMatrix;

        AssetManager::GetInstance().ParseModelData(FilePath, modeldata);
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


//const std::vector<ModelVertex>& MeshLoader::GetVertices() {
//    return vertices;
//}
//
//const std::vector<uint32_t >& MeshLoader::GetIndices() {
//    return indices;
//}


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