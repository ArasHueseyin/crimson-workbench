#pragma once
#include "teleport_protocol.h"
#include <Windows.h>
#include <wincrypt.h>
#include <ShlObj.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>

namespace crimson::marketplace::teleport {
// Only the IPC worker calls this store. There are no engine pointers or save
// files here. DPAPI protects the native-captured bookmark payload against
// accidental edits that would turn a stored height into arbitrary input XYZ.
class BookmarkStore {
    std::filesystem::path path_;
    std::array<Bookmark,kSavedSlots> last_{};
    struct Blob {std::uint32_t magic{kMagic},version{1};std::array<char,65> build{};std::array<char,7> reserved{};std::array<Bookmark,kSavedSlots> bookmarks{};};
    static_assert(sizeof(Blob)==1488);
public:
    BookmarkStore(){
        PWSTR local{};
        if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_DEFAULT,nullptr,&local))&&local)path_=std::filesystem::path(local)/L"CrimsonWorkbench"/L"teleport-bookmarks.bin";
        CoTaskMemFree(local);
    }
    // Owned fixture stores can use an isolated path without touching AppData.
    explicit BookmarkStore(std::filesystem::path path):path_(std::move(path)){}
    std::array<Bookmark,kSavedSlots> load(const char* build) {
        if(path_.empty())return last_;
        std::ifstream file(path_,std::ios::binary|std::ios::ate);if(!file)return last_;
        const auto size=file.tellg();if(size<=0||size>8192)return last_;
        std::vector<unsigned char> storage(static_cast<std::size_t>(size));file.seekg(0);if(!file.read(reinterpret_cast<char*>(storage.data()),size))return last_;
        DATA_BLOB input{static_cast<DWORD>(storage.size()),storage.data()},output{};
        if(!CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))return last_;
        Blob value{};const bool valid=output.cbData==sizeof(Blob);
        if(valid)std::memcpy(&value,output.pbData,sizeof(value));LocalFree(output.pbData);
        if(valid&&value.magic==kMagic&&value.version==1&&!std::memcmp(value.build.data(),build,65)&&value.reserved==std::array<char,7>{})last_=value.bookmarks;
        return last_;
    }
    bool save(const std::array<Bookmark,kSavedSlots>& values,const char* build) {
        if(path_.empty())return false;
        // PersistenceRevision suppresses polling writes. An explicit new
        // capture/removal must recreate storage even if its values match the
        // in-memory snapshot and the file was meanwhile removed/replaced.
        Blob value{};std::memcpy(value.build.data(),build,65);value.bookmarks=values;
        DATA_BLOB input{sizeof(value),reinterpret_cast<BYTE*>(&value)},output{};
        if(!CryptProtectData(&input,L"Crimson Workbench teleport bookmarks",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))return false;
        bool written=false;
        try {
            std::filesystem::create_directories(path_.parent_path());auto temporary=path_;temporary+=L".tmp";
            {std::ofstream file(temporary,std::ios::binary|std::ios::trunc);written=bool(file.write(reinterpret_cast<const char*>(output.pbData),output.cbData));file.close();written=written&&!file.fail();}
            if(written)written=MoveFileExW(temporary.c_str(),path_.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
        } catch(...){written=false;}
        LocalFree(output.pbData);if(written)last_=values;return written;
    }
};
} // namespace crimson::marketplace::teleport
