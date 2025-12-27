#include "ZipUtil.h"
extern "C" {
#include "zip.h" // minizip
}
#include <cstring>

bool saveLevelZip(const std::string& zipPath, const std::string& layout, const std::string& assets){
    zipFile zf = zipOpen(zipPath.c_str(), 0);
    if(!zf) return false;

    zip_fileinfo zi{};
    if(zipOpenNewFileInZip(zf, "layout.txt", &zi, nullptr,0,nullptr,0,nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK){
        zipClose(zf, nullptr); return false;
    }
    zipWriteInFileInZip(zf, layout.data(), (unsigned)layout.size());
    zipCloseFileInZip(zf);

    if(zipOpenNewFileInZip(zf, "assets.txt", &zi, nullptr,0,nullptr,0,nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK){
        zipClose(zf, nullptr); return false;
    }
    zipWriteInFileInZip(zf, assets.data(), (unsigned)assets.size());
    zipCloseFileInZip(zf);

    zipClose(zf, nullptr);
    return true;
}