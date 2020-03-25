#include "FileReader.h"

AAssetManager* FileReader::m_assetManager = nullptr;

void FileReader::setup(AAssetManager* assetManager)
{
	FileReader::m_assetManager = assetManager;
}

std::vector<char> FileReader::readData(const char *relativePath)
{
	AAsset* asset = AAssetManager_open(FileReader::m_assetManager, relativePath, AASSET_MODE_BUFFER);
	std::vector<char> data(static_cast<size_t>(AAsset_getLength(asset)));
	AAsset_read(asset, data.data(), data.size());
	AAsset_close(asset);

	return data;
}

