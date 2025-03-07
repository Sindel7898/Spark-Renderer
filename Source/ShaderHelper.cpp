#include"ShaderHelper.h"


std::vector<char> readFile(const std::string& filepath)
{
	//ate: Start reading at the end of the file
	//binary : Read the file as binary file(avoid text transformations)
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file " + filepath);
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
