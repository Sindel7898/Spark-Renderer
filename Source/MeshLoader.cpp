#include "MeshLoader.h"
#include <iostream>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"
#include "AssetManager.h"
#include <glm/gtc/type_ptr.hpp>
#include "meshoptimizer.h"
#include <thread>
#include <future>
#include <mutex>
#include <chrono>  

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

    using Clock = std::chrono::high_resolution_clock;

    auto start = Clock::now();


    auto materialFuture = std::async(std::launch::async, [this, pFile, &model]() mutable {
             LoadMaterials(pFile, model);
        });


    std::cout << "Loaded glTF: " << pFile << std::endl;


    StoredModelData modelData;

    const tinygltf::Scene& scene = model.scenes[0];

    for (size_t i = 0; i < scene.nodes.size(); i++) {

        const tinygltf::Node node = model.nodes[scene.nodes[i]];

        if (auto rootNode = loadNode(node, model, modelData.VertexData, modelData.IndexData, nullptr)) {

            modelData.nodes.push_back(std::move(rootNode));
        }
    }

    auto end = Clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Model loaded in " << elapsed.count() << " seconds.\n";

    std::vector<unsigned int> remap(modelData.IndexData.size());

    meshopt_generateVertexRemap(remap.data(),
        modelData.IndexData.data(), modelData.IndexData.size(),
        modelData.VertexData.data(), modelData.VertexData.size(),
        sizeof(ModelVertex));

    StoredModelData Opt_modelData;

    // Resize vectors BEFORE using data()
    Opt_modelData.IndexData.resize(modelData.IndexData.size());
    Opt_modelData.VertexData.resize(modelData.VertexData.size());

    meshopt_remapIndexBuffer(Opt_modelData.IndexData.data(),
        modelData.IndexData.data(), modelData.IndexData.size(),
        remap.data());

    meshopt_remapVertexBuffer(Opt_modelData.VertexData.data(),
        modelData.VertexData.data(), modelData.VertexData.size(),
        sizeof(ModelVertex), remap.data());


    meshopt_optimizeVertexCache(Opt_modelData.IndexData.data(), Opt_modelData.IndexData.data(), Opt_modelData.IndexData.size(), modelData.VertexData.size());

    meshopt_optimizeVertexFetch(Opt_modelData.VertexData.data(), Opt_modelData.IndexData.data(), Opt_modelData.IndexData.size(), Opt_modelData.VertexData.data(), Opt_modelData.VertexData.size(), sizeof(ModelVertex));
    


    materialFuture.get();
    Opt_modelData.nodes = modelData.nodes;

    AssetManager::GetInstance().ParseModelData(pFile, Opt_modelData);

}


void MeshLoader::LoadMaterials(const std::string& pFile, tinygltf::Model& model)
{
    if (!model.textures.empty()) {

        //create the list of textures array
        std::vector<StoredImageData> Textures;
        Textures.reserve(model.materials.size() * 4);


        // For all the "Materials group" get the individual material  
        for (int i = 0; i < model.materials.size(); i++)
        {
             tinygltf::Material gltfMaterial = model.materials[i];
           
              auto ColorTextureTask = std::async(std::launch::async, [this, gltfMaterial, &model]() {
                                      return ReadTexture(gltfMaterial, "baseColorTexture", model);
               });

              auto NormalTextureTask = std::async(std::launch::async, [this, gltfMaterial, &model]() {
                  return ReadTexture(gltfMaterial, "normalTexture", model);
                  });

              auto MetalicRoughnessTextureTask = std::async(std::launch::async, [this, gltfMaterial, &model]() {
                  return ReadTexture(gltfMaterial, "metallicRoughnessTexture", model);
                  });


              auto OcculusionTextureTask = std::async(std::launch::async, [this, gltfMaterial, &model]() {
                  return ReadTexture(gltfMaterial, "occlusionTexture", model);
                  });


              auto EmissiveTextureTask = std::async(std::launch::async, [this, gltfMaterial, &model]() {
                  return ReadTexture(gltfMaterial, "emissiveTexture", model);
                  });

              
              Textures.push_back(std::move(ColorTextureTask.get()));
              Textures.push_back(std::move(NormalTextureTask.get()));
              Textures.push_back(std::move(MetalicRoughnessTextureTask.get()));
              Textures.push_back(std::move(OcculusionTextureTask.get()));
              Textures.push_back(std::move(EmissiveTextureTask.get()));
        }

        AssetManager::GetInstance().ParseTextureData(pFile, Textures);
    }
    else {
        std::cout << "No textures found in the model.\n";
    }
}

StoredImageData MeshLoader::ReadTexture(const tinygltf::Material& gltfMaterial, std::string TextureType, tinygltf::Model& model) {

    {
        auto value = gltfMaterial.values.find(TextureType);

        if (value != gltfMaterial.values.end()) {
          
            StoredImageData TextureData;
            //get the texture from the material map
            tinygltf::Texture& colortex = model.textures[value->second.TextureIndex()];
            //get the image from the texture
            const tinygltf::Image& image = model.images[colortex.source];

            size_t colorimageSize = image.width * image.height * 4;

            TextureData.imageData = static_cast<stbi_uc*>(malloc(colorimageSize));

            std::memcpy(TextureData.imageData, image.image.data(), colorimageSize);

            TextureData.imageHeight = image.height;
            TextureData.imageWidth = image.width;

            return TextureData;
        }

        auto additionalvalue = gltfMaterial.additionalValues.find(TextureType);

        if (additionalvalue != gltfMaterial.additionalValues.end()) {

            StoredImageData TextureData;
            //get the texture from the material map
            tinygltf::Texture& colortex = model.textures[additionalvalue->second.TextureIndex()];
            //get the image from the texture
            const tinygltf::Image& image = model.images[colortex.source];

            size_t colorimageSize = image.width * image.height * 4;

            TextureData.imageData = static_cast<stbi_uc*>(malloc(colorimageSize));

            std::memcpy(TextureData.imageData, image.image.data(), colorimageSize);

            TextureData.imageHeight = image.height;
            TextureData.imageWidth = image.width;

            return TextureData;
        }

         std::vector<stbi_uc> DefaultImage;
         const int ImageSize = 4;
         
         DefaultImage.resize(ImageSize * ImageSize * 4);
         
         if (TextureType == "emissiveTexture")
         {
             for (int i = 0; i < ImageSize * ImageSize * 4; i += 4) {

                 DefaultImage[i + 0] = 0;   // R
                 DefaultImage[i + 1] = 0;   // G
                 DefaultImage[i + 2] = 0;   // B
                 DefaultImage[i + 3] = 255; // A

             }
         }
         else
         {
             for (int i = 0; i < ImageSize * ImageSize * 4; i += 4) {
                 DefaultImage[i + 0] = 255; // R
                 DefaultImage[i + 1] = 255; // G
                 DefaultImage[i + 2] = 255; // B
                 DefaultImage[i + 3] = 255; // A
             }
         }
         
         size_t DefaultimageSize = DefaultImage.size();
         
         StoredImageData TextureData;
         TextureData.imageData = static_cast<stbi_uc*>(malloc(DefaultimageSize));
         
         // Copy pixel data
         std::memcpy(TextureData.imageData, DefaultImage.data(), DefaultimageSize);
         
         TextureData.imageHeight = ImageSize;
         TextureData.imageWidth = ImageSize;
         
         return TextureData; 
    }
}

std::unique_ptr<Node> MeshLoader::loadNode(const tinygltf::Node&      inputNode, 
                                           const tinygltf::Model&     model,
                                           std::vector<ModelVertex>&  vertices, 
                                           std::vector<uint32_t >&    indices,
                                                              Node*   parent)
{

    std::unique_ptr<Node> node = std::make_unique<Node>();;
    node->parent = parent;
    node->matrix  = glm::mat4(1.0f);

  
    if (inputNode.translation.size() == 3)
    {
        node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
    }

    if (inputNode.rotation.size() == 4)
    {
        glm::quat q = glm::make_quat(inputNode.rotation.data());
        node->matrix *= glm::mat4(q);
    }

    if (inputNode.scale.size() == 3)
    {
        node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
    }
    
    if (inputNode.matrix.size() == 16)
    {
        node->matrix = glm::make_mat4x4(inputNode.matrix.data());
    }


    if (inputNode.mesh > -1)
    {
        const tinygltf::Mesh mesh = model.meshes[inputNode.mesh];

        for (size_t i = 0; i < mesh.primitives.size(); i++) {

            const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
            
            node->meshPrimitives.push_back(ProcessPrimitive(glTFPrimitive, model, vertices, indices));
        }
    }

    for (int childIndex : inputNode.children) {

        if (auto rootNode = loadNode(model.nodes[childIndex], model, vertices, indices, node.get())) {

            node->children.push_back(std::move(rootNode));
        }

    }

    return node;
}

Primitive MeshLoader::ProcessPrimitive(const tinygltf::Primitive& glTFPrimitive, const tinygltf::Model& model, std::vector<ModelVertex>& outVertices, std::vector<uint32_t>& outIndices) {

            uint32_t indicesStart = static_cast<uint32_t>(outIndices.size());
            uint32_t verticesStart = static_cast<uint32_t>(outVertices.size());
           
            uint32_t indexCount  = 0;
            uint32_t vertexCount = 0;

         
             const float* positions = nullptr;
             const float* normals = nullptr;
             const float* texcoords = nullptr;
             const float* tangents = nullptr;

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

            if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end()) {
                 
                 const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
                 const tinygltf::BufferView& view   = model.bufferViews[accessor.bufferView];

                tangents = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
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

                if (tangents) {
                     vertex.tangent = { tangents[i * 3 + 0], tangents[i * 3 + 1],tangents[i * 3 + 2] };
                 }

                outVertices.push_back(vertex);
             }

             const auto& indexAccessor = model.accessors[glTFPrimitive.indices];
             const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
             const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            indexCount += static_cast<uint32_t>(indexAccessor.count);

            switch (indexAccessor.componentType) {
             case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                 const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[indexAccessor.byteOffset + bufferView.byteOffset]);
                 for (size_t index = 0; index < indexAccessor.count; index++) {
                     outIndices.push_back(buf[index] + verticesStart);
                 }
                 break;
             }
             case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                 const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[indexAccessor.byteOffset + bufferView.byteOffset]);
                 for (size_t index = 0; index < indexAccessor.count; index++) {
                     outIndices.push_back(buf[index] + verticesStart);
                 }
                 break;
             }
             case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                 const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[indexAccessor.byteOffset + bufferView.byteOffset]);
                 for (size_t index = 0; index < indexAccessor.count; index++) {
                     outIndices.push_back(buf[index] + verticesStart);
                 }
                 break;
             }
             default:
                 std::cerr << "Index component type " << indexAccessor.componentType << " not supported!" << std::endl;
             }
          


            Primitive primitive;
            primitive.indicesStart   = indicesStart;
            primitive.numIndices     = indexCount;
            primitive.materialIndex  = glTFPrimitive.material;
            return primitive;
}