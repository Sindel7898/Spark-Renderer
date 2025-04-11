#include "AssetManager.h"
#include "MeshLoader.h"

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

void AssetManager::LoadTexture(const std::string& filePath)
{
	int texWidth;
	int textHeight;
	int textchannels;

	//// If File Path is not in the Map Add it and the Data
	if (LoadedTextureData.find(filePath.c_str()) == LoadedTextureData.end()) {

		stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &textHeight, &textchannels, STBI_rgb_alpha);

		StoredImageData infotostore{};
		infotostore.imageData = pixels;
		infotostore.imageWidth = texWidth;
		infotostore.imageHeight = textHeight;

		LoadedTextureData.emplace(filePath, infotostore);
	}

}

const StoredImageData& AssetManager::GetStoredImageData(const std::string FilePath)
{
	auto FoundData = LoadedTextureData.find(FilePath);

	if (FoundData == LoadedTextureData.end()) {
		// Either load the data or throw an exception
		throw std::runtime_error("Texture data not found for path: " + FilePath);
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
		if (MapData.second.imageData) {
			stbi_image_free(MapData.second.imageData);
			MapData.second.imageData = nullptr;
		}
	}

	for (auto& MapData : LoadedModelData)
	{
		MapData.second.IndexData.clear();
		MapData.second.VertexData.clear();
	}
}
