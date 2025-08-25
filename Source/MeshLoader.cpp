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

    
    const tinygltf::Scene& scene = model.scenes[0];

    for (size_t i = 0; i < scene.nodes.size(); i++) {

        const tinygltf::Node node = model.nodes[scene.nodes[i]];

        ProcessNode(node, model);
    }
    

}

void MeshLoader::LoadMaterials(const std::string& pFile, tinygltf::Model& model)
{
    if (!model.textures.empty()) {

        //create the list of textures array
        std::vector<StoredImageData> Textures(4);

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

                   TextureData.imageData = static_cast<stbi_uc*>(malloc(colorimageSize));

                   std::memcpy(TextureData.imageData, image.image.data(), colorimageSize);

                   TextureData.imageHeight = image.height;
                   TextureData.imageWidth = image.width;
               
                   Textures[0] = TextureData;
               }
               else
               {
                   std::vector<stbi_uc> DefaultImage;
                   const int ImageSize = 4;

                   DefaultImage.resize(ImageSize * ImageSize * 4);

                   for (int i = 0; i < ImageSize * ImageSize * 4; i += 4) {

                       DefaultImage[i + 0] = 255; // R
                       DefaultImage[i + 1] = 255; // G
                       DefaultImage[i + 2] = 255; // B
                       DefaultImage[i + 3] = 255; // A
                   }

                   size_t DefaultimageSize = DefaultImage.size();

                   StoredImageData TextureData;
                   TextureData.imageData = static_cast<stbi_uc*>(malloc(DefaultimageSize));

                   // Copy pixel data
                   std::memcpy(TextureData.imageData, DefaultImage.data(), DefaultimageSize);

                   TextureData.imageHeight = ImageSize;
                   TextureData.imageWidth = ImageSize;

                   // Store in your textures array
                   Textures[0] = TextureData;
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
                   TextureData.imageData = static_cast<stbi_uc*>(malloc(normalimageSize));
                   std::memcpy(TextureData.imageData, image.image.data(), normalimageSize);
                   TextureData.imageHeight = image.height;
                   TextureData.imageWidth = image.width;
               
                   Textures[1] = TextureData;
               }
               else
               {
                   std::vector<stbi_uc> DefaultImage;
                   const int ImageSize = 4;

                   DefaultImage.resize(ImageSize * ImageSize * 4);

                   for (int i = 0; i < ImageSize * ImageSize * 4; i += 4) {

                       DefaultImage[i + 0] = 255; // R
                       DefaultImage[i + 1] = 255; // G
                       DefaultImage[i + 2] = 255; // B
                       DefaultImage[i + 3] = 255; // A
                   }

                   size_t DefaultimageSize = DefaultImage.size();

                   StoredImageData TextureData;
                   TextureData.imageData = static_cast<stbi_uc*>(malloc(DefaultimageSize));

                   // Copy pixel data
                   std::memcpy(TextureData.imageData, DefaultImage.data(), DefaultimageSize);

                   TextureData.imageHeight = ImageSize;
                   TextureData.imageWidth = ImageSize;

                   // Store in your textures array
                   Textures[1] = TextureData;
               }
            }

            {
                //check if the material group has the normal texture in its map. 
                if (gltfMaterial.values.find("metallicRoughnessTexture") != gltfMaterial.values.end())
                {
                    StoredImageData TextureData;
                    //get the texture from the material map
                    tinygltf::Texture& metalicroughnesstex = model.textures[gltfMaterial.values["metallicRoughnessTexture"].TextureIndex()];
                    //get the image from the texture
                    const tinygltf::Image& image = model.images[metalicroughnesstex.source];

                    size_t metalicroughnessimageSize = image.width * image.height * 4;
                    TextureData.imageData = static_cast<stbi_uc*>(malloc(metalicroughnessimageSize));
                    std::memcpy(TextureData.imageData, image.image.data(), metalicroughnessimageSize);
                    TextureData.imageHeight = image.height;
                    TextureData.imageWidth = image.width;

                    Textures[2] = TextureData;
                }
                else
                {
                    std::vector<stbi_uc> DefaultImage;
                    const int ImageSize = 4;

                    DefaultImage.resize(ImageSize* ImageSize * 4);

                    for (int i = 0; i < ImageSize * ImageSize * 4; i += 4) {

                        DefaultImage[i + 0] = 255; // R
                        DefaultImage[i + 1] = 255; // G
                        DefaultImage[i + 2] = 255; // B
                        DefaultImage[i + 3] = 255; // A
                    }

                    size_t DefaultimageSize = DefaultImage.size();

                    StoredImageData TextureData;
                    TextureData.imageData = static_cast<stbi_uc*>(malloc(DefaultimageSize));

                    // Copy pixel data
                    std::memcpy(TextureData.imageData, DefaultImage.data(), DefaultimageSize);

                    TextureData.imageHeight = ImageSize;
                    TextureData.imageWidth = ImageSize;

                    // Store in your textures array
                    Textures[2] = TextureData;
                }
            }

            {
                //check if the material group has the normal texture in its map. 
                if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end())
                {
                    StoredImageData TextureData;
                    //get the texture from the material map
                    tinygltf::Texture& occlusiontex = model.textures[gltfMaterial.additionalValues["occlusionTexture"].TextureIndex()];
                    //get the image from the texture
                    const tinygltf::Image& image = model.images[occlusiontex.source];

                    size_t occlusionmageSize = image.width * image.height * 4;
                    TextureData.imageData = static_cast<stbi_uc*>(malloc(occlusionmageSize));
                    std::memcpy(TextureData.imageData, image.image.data(), occlusionmageSize);
                    TextureData.imageHeight = image.height;
                    TextureData.imageWidth = image.width;

                    Textures[3] = TextureData;
                }
                else
                {
                    std::vector<stbi_uc> DefaultImage;
                    const int ImageSize = 4;

                    DefaultImage.resize(ImageSize* ImageSize * 4);

                    for (int i = 0; i < ImageSize * ImageSize * 4; i += 4) {

                        DefaultImage[i + 0] = 255; // R
                        DefaultImage[i + 1] = 255; // G
                        DefaultImage[i + 2] = 255; // B
                        DefaultImage[i + 3] = 255; // A
                    }

                    size_t DefaultimageSize = DefaultImage.size();

                    StoredImageData TextureData;
                    TextureData.imageData = static_cast<stbi_uc*>(malloc(DefaultimageSize));

                    // Copy pixel data
                    std::memcpy(TextureData.imageData, DefaultImage.data(), DefaultimageSize);

                    TextureData.imageHeight = ImageSize;
                    TextureData.imageWidth = ImageSize;

                    // Store in your textures array
                    Textures[3] = TextureData;
                }
            }


            //{
            //    //check if the material group has the normal texture in its map. 
            //    if (gltfMaterial.additionalValues.find("HeightMap") != gltfMaterial.additionalValues.end())
            //    {
            //        StoredImageData TextureData;
            //        //get the texture from the material map
            //        tinygltf::Texture& HeightMaptex = model.textures[gltfMaterial.additionalValues["HeightMap"].TextureIndex()];
            //        //get the image from the texture
            //        const tinygltf::Image& image = model.images[HeightMaptex.source];
            //
            //        size_t HeightMapteximageSize = image.width * image.height * 4;
            //        TextureData.imageData = static_cast<stbi_uc*>(malloc(HeightMapteximageSize));
            //        std::memcpy(TextureData.imageData, image.image.data(), HeightMapteximageSize);
            //        TextureData.imageHeight = image.height;
            //        TextureData.imageWidth = image.width;
            //
            //        Textures.push_back(TextureData);
            //    }
            //}
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
            const float* tangents = nullptr;

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

            if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
                const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];

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
}
