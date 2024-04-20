// NetCDFReader.h
#ifndef NETCDFREADER_H
#define NETCDFREADER_H

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <netcdf>
#include <string>
#include <vector>

class NetCDFReader {
public:
    NetCDFReader(AAssetManager* assetManager, const std::string& filename);
    void printVariableNames() const;
    const std::vector<std::string>& getVariableNames() const;

private:
    AAssetManager* mAssetManager;
    std::string mFilename;
    std::vector<std::string> variableNames;
};

#endif // NETCDFREADER_H
