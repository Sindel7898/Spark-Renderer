#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include "MeshLoader.h"


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
    void ParseTextureData(const std::string FilePath, std::vector<StoredImageData> Textures);
    const StoredModelData& GetStoredModelData(const std::string FilePath);

    //////////////////////////////////////////////////////////
    const std::vector<StoredImageData>& GetStoredImageData(const std::string MeshFilePath);


private:
    AssetManager();
    ~AssetManager();

    std::shared_ptr<MeshLoader> meshloader;
    std::map<std::string, StoredModelData> LoadedModelData;
    std::map<std::string, std::vector<StoredImageData> > LoadedTextureData;
};