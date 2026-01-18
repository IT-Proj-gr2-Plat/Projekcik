#include "ZipUtil.h"
extern "C" {
#include "zip.h" // minizip
#include "unzip.h"
}
#include <cstring>
#include <fstream>
#include <vector>

bool saveLevelZip(const std::string& zipPath, const std::string& layout, const std::string& assets, const std::string& backgroundPath){
    zipFile zf = zipOpen(zipPath.c_str(), 0);
    if(!zf) return false;

    zip_fileinfo zi{};

    // Add layout.txt
    if(zipOpenNewFileInZip(zf, "layout.txt", &zi, nullptr,0,nullptr,0,nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK){
        zipClose(zf, nullptr); return false;
    }
    zipWriteInFileInZip(zf, layout.data(), (unsigned)layout.size());
    zipCloseFileInZip(zf);

    // Add assets.txt
    if(zipOpenNewFileInZip(zf, "assets.txt", &zi, nullptr,0,nullptr,0,nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK){
        zipClose(zf, nullptr); return false;
    }
    zipWriteInFileInZip(zf, assets.data(), (unsigned)assets.size());
    zipCloseFileInZip(zf);

    // Add background file if exists
    std::ifstream bgFile(backgroundPath, std::ios::binary);
    if (bgFile) {
        std::string bgData((std::istreambuf_iterator<char>(bgFile)), std::istreambuf_iterator<char>());
        std::string bgFilename = backgroundPath.substr(backgroundPath.find_last_of("/\\") + 1);
        if(zipOpenNewFileInZip(zf, bgFilename.c_str(), &zi, nullptr,0,nullptr,0,nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK){
            zipClose(zf, nullptr); return false;
        }
        zipWriteInFileInZip(zf, bgData.data(), (unsigned)bgData.size());
        zipCloseFileInZip(zf);
    }

    zipClose(zf, nullptr);
    return true;
}

bool loadLevelZip(const std::string& zipPath, std::string& layout, std::string& assets) {
    unzFile uf = unzOpen(zipPath.c_str());
    if (!uf) return false;

    // Extract layout.txt
    if (unzLocateFile(uf, "layout.txt", 0) == UNZ_OK) {
        unz_file_info fileInfo;
        if (unzGetCurrentFileInfo(uf, &fileInfo, nullptr, 0, nullptr, 0, nullptr, 0) == UNZ_OK) {
            if (unzOpenCurrentFile(uf) == UNZ_OK) {
                std::vector<char> buffer(fileInfo.uncompressed_size);
                int read = unzReadCurrentFile(uf, buffer.data(), fileInfo.uncompressed_size);
                if (read == fileInfo.uncompressed_size) {
                    layout.assign(buffer.begin(), buffer.end());
                }
                unzCloseCurrentFile(uf);
            }
        }
    }

    // Extract assets.txt
    if (unzLocateFile(uf, "assets.txt", 0) == UNZ_OK) {
        unz_file_info fileInfo;
        if (unzGetCurrentFileInfo(uf, &fileInfo, nullptr, 0, nullptr, 0, nullptr, 0) == UNZ_OK) {
            if (unzOpenCurrentFile(uf) == UNZ_OK) {
                std::vector<char> buffer(fileInfo.uncompressed_size);
                int read = unzReadCurrentFile(uf, buffer.data(), fileInfo.uncompressed_size);
                if (read == fileInfo.uncompressed_size) {
                    assets.assign(buffer.begin(), buffer.end());
                }
                unzCloseCurrentFile(uf);
            }
        }
    }

    unzClose(uf);
    return !layout.empty() && !assets.empty();
}
