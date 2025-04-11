#pragma once

#include "assimp\Importer.hpp"      
#include "assimp\scene.h"          
#include "assimp\postprocess.h"     
#include "VertexInputLayouts.h"          
#include <vector>


class MeshLoader
{
public:

	MeshLoader();
	void LoadModel(const std::string& pFile);

	const std::vector<ModelVertex>& GetVertices();
	const std::vector<uint16_t>& GetIndices();

private:
	Assimp::Importer importer;
	void ProcessNode(aiNode* node, const aiScene* scene);
	void ProcessMesh(aiMesh* mesh, const aiScene* scene);


	std::vector<ModelVertex> vertices;
	std::vector<uint16_t> indices;
};