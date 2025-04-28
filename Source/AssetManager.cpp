#include "AssetManager.h"
#include "MeshLoader.h"
#include "stb_image.h"

AssetManager::AssetManager() {
	meshloader = std::make_shared<MeshLoader>();
}

void AssetManager::LoadModel(const std::string& filePath)
{
	meshloader = std::make_shared<MeshLoader>();

	//// If File Path is not in the Map Add it and the Data
	if (LoadedModelData.find(filePath) == LoadedModelData.end()) {

		meshloader->LoadModel(filePath);
		StoredModelData infotostore{};

		infotostore.VertexData = meshloader->GetVertices();
		infotostore.IndexData = meshloader->GetIndices();

		LoadedModelData.emplace(filePath, infotostore);
	}
}

void AssetManager::ParseTextureData(const std::string FilePath, std::vector<StoredImageData> Textures)
{
	//// If File Path is not in the Map Add it and the Data
	if (LoadedTextureData.find(FilePath.c_str()) == LoadedTextureData.end()) {

		LoadedTextureData.emplace(FilePath, Textures);
	}
}

const std::vector<StoredImageData>& AssetManager::GetStoredImageData(const std::string MeshFilePath)
{
	auto FoundData = LoadedTextureData.find(MeshFilePath);

	if (FoundData == LoadedTextureData.end()) {
		// Either load the data or throw an exception
		throw std::runtime_error("Texture data not found for path: " + MeshFilePath);
	}


	return FoundData->second;
}

const StoredModelData& AssetManager::GetStoredModelData(const std::string FilePath)
{
	auto FoundData = LoadedModelData.find(FilePath);

	if (FoundData == LoadedModelData.end()) {
		// Either load the data or throw an exception
		throw std::runtime_error("Model data not found for path: " + FilePath);
	}

	return FoundData->second;
}


AssetManager::~AssetManager() {

	
	for (auto& MapData : LoadedTextureData)
	{

		for (int i = 0; i < MapData.second.size(); i++)
		{
			if (MapData.second[i].imageData) {
				stbi_image_free(MapData.second[i].imageData);
			}
		}
	}
	LoadedTextureData.clear();

	for (auto& MapData : LoadedModelData)
	{
		MapData.second.IndexData.clear();
		MapData.second.VertexData.clear();
	}

	LoadedModelData.clear();

}
