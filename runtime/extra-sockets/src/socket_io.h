#pragma once
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <string>
#include <vector>
#include "socket_policy.h"
namespace extra {
inline bool digest(const void* data,std::size_t size,unsigned char* out){
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE h{};bool ok=false;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptCreateHash(alg,&h,nullptr,0,nullptr,0,0)>=0)
        ok=BCryptHashData(h,(PUCHAR)data,static_cast<ULONG>(size),0)>=0&&BCryptFinishHash(h,out,32,0)>=0;
    if(h)BCryptDestroyHash(h);if(alg)BCryptCloseAlgorithmProvider(alg,0);return ok;
}
inline bool seal(Config& config){return structure(config)&&digest(config.records,config.count*sizeof(Record),config.digest);}
inline bool checked(const Config& config){unsigned char hash[32];return structure(config)&&digest(config.records,config.count*sizeof(Record),hash)&&!std::memcmp(hash,config.digest,32);}
inline bool load(const std::wstring& path,Config& out){
    HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(f==INVALID_HANDLE_VALUE)return false;
    out={};BY_HANDLE_FILE_INFORMATION info{};DWORD n=0;bool ok=GetFileInformationByHandle(f,&info)
        &&!(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))&&info.nNumberOfLinks==1
        &&info.nFileSizeHigh==0&&info.nFileSizeLow>=48&&info.nFileSizeLow<=sizeof(out)
        &&ReadFile(f,&out,info.nFileSizeLow,&n,nullptr)&&n==info.nFileSizeLow&&out.count<=maxRecords&&configSize(out)==n;
    CloseHandle(f);return ok&&checked(out);
}
inline bool persist(const std::wstring& path,const Config& before,Config& after){
    Config current{};if(!load(path,current)||std::memcmp(&current,&before,sizeof(current))||!seal(after))return false;
    std::wstring temp=path+L".writing";HANDLE f=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE)return false;DWORD n=0;
    bool ok=WriteFile(f,&after,configSize(after),&n,nullptr)&&n==configSize(after)&&FlushFileBuffers(f);CloseHandle(f);
    if(ok)ok=load(path,current)&&!std::memcmp(&current,&before,sizeof(current));
    if(ok){const auto backup=path+L".previous";ok=CopyFileW(path.c_str(),backup.c_str(),FALSE)!=0;}
    if(ok)ok=MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temp.c_str());return ok;
}
}
