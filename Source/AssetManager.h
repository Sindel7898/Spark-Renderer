#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include "stb_image.h"
#include "MeshLoader.h"

struct StoredImageData
{
    stbi_uc* imageData;
    int      imageHeight;
    int      imageWidth;

};

struct StoredModelData
{
    std::vector<ModelVertex> VertexData;
    std::vector<uint16_t>    IndexData;
};


class AssetManager {
public:
    
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

     static AssetManager& GetInstance()
      {
          static AssetManager instance;
          return instance;
      }

    void LoadModel(const std::string& filePath);
    const StoredModelData& GetStoredModelData(const std::string FilePath);

    //////////////////////////////////////////////////////////
    void LoadTexture(const std::string& filePath);
    const StoredImageData& GetStoredImageData(const std::string FilePath);


private:
    AssetManager();
    ~AssetManager();

    std::shared_ptr<MeshLoader> meshloader;
    std::map<std::string, StoredModelData> LoadedModelData;
    std::map<std::string, StoredImageData> LoadedTextureData;
};