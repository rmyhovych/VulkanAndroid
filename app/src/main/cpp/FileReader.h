#pragma once
#include <vector>
#include <string>
#include <android_native_app_glue.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

class FileReader
{
public:
	static void setup(AAssetManager* assetManager);
	static std::vector<char> readData(const char* relativePath);

private:
	FileReader() {}
	static AAssetManager* m_assetManager;
};
