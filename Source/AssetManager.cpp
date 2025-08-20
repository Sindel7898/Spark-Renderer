#include "AssetManager.h"
#include "MeshLoader.h"
#include "stb_image.h"

AssetManager::AssetManager() {
	meshloader = std::make_shared<MeshLoader>();
}



void AssetManager::ParseModelData(const std::string& filePath, StoredModelData modeldata)
{
	//// If file path is not in the Map Add it and the Data
	if (LoadedModelData.find(filePath) == LoadedModelData.end()) {
		
		LoadedModelData.emplace(filePath, modeldata);
	}
}

void AssetManager::ParseTextureData(const std::string& filePath, std::vector<StoredImageData> Textures)
{
	//// If file path is not in the Map Add it and the Data
	if (LoadedTextureData.find(filePath) == LoadedTextureData.end()) {

		LoadedTextureData.emplace(filePath, Textures);
	}
}

const std::vector<StoredImageData>& AssetManager::GetStoredImageData(const std::string& MeshFilePath)
{
	// find the data associated to the filepath and return it 
	auto FoundData = LoadedTextureData.find(MeshFilePath);

	if (FoundData == LoadedTextureData.end()) {

		throw std::runtime_error("Texture data not found for path: " + MeshFilePath);
	}

	return FoundData->second;
}

const StoredModelData& AssetManager::GetStoredModelData(const std::string& FilePath)
{
	// find the data associated to the filepath and return it 

	auto found = LoadedModelData.find(FilePath);

	// if there is no text that matches the file path in the map load the model 
	// once the model is loaded find the filepath in the map and return it
	if (found == LoadedModelData.end()) {

		meshloader->LoadModel(FilePath);

		found = LoadedModelData.find(FilePath);

		if (found == LoadedModelData.end()) {
			throw std::runtime_error("Model data could not be loaded for path: " + FilePath);
		}

	}

	return found->second;
}


AssetManager::~AssetManager() {
	// clean up
	for (auto& [path, textures] : LoadedTextureData) {

		for (auto& tex : textures) {

			if (tex.imageData) {

				stbi_image_free(tex.imageData);

				tex.imageData = nullptr;
			}
		}
	}

	LoadedTextureData.clear();

	for (auto& [path, model] : LoadedModelData) {

		model.VertexData.clear();
		model.IndexData.clear();

	}
	LoadedModelData.clear();
}
