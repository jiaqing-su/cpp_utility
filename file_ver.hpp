

#ifndef _FILE_VER_HPP_
#define _FILE_VER_HPP_

#include <winver.h>
#include <string>
#include <sstream>
#include <stdint.h>

#pragma comment(lib, "Version.lib")

namespace sj
{
namespace file
{

inline uint64_t GetFileVersion(const std::wstring &lpFilePath)
{
    UINT64 u64Ver = 0;
    DWORD dwSize = 0;
    LPVOID pData = NULL;
    VS_FIXEDFILEINFO *verInfo = NULL;
    UINT dwDataLen = 0;

    dwSize = GetFileVersionInfoSize(lpFilePath.c_str(), NULL);
    pData = malloc(dwSize);
    if (pData)
    {
        if (GetFileVersionInfo(lpFilePath.c_str(), NULL, dwSize, pData))
        {
            if (VerQueryValue(pData, L"\\", (void **)&verInfo, &dwDataLen))
            {
                if (verInfo)
                {
                    u64Ver = verInfo->dwFileVersionMS;
                    u64Ver <<= 32;
                    u64Ver |= verInfo->dwFileVersionLS;
                }
            }
        }
        free(pData);
    }

    return u64Ver;
}


inline uint64_t GetFileVersion(const std::string& lpFilePath)
{
    UINT64 u64Ver = 0;
    DWORD dwSize = 0;
    LPVOID pData = NULL;
    VS_FIXEDFILEINFO* verInfo = NULL;
    UINT dwDataLen = 0;

    dwSize = GetFileVersionInfoSizeA(lpFilePath.c_str(), NULL);
    pData = malloc(dwSize);
    if (pData)
    {
        if (GetFileVersionInfoA(lpFilePath.c_str(), NULL, dwSize, pData))
        {
            if (VerQueryValueA(pData, "\\", (void**)&verInfo, &dwDataLen))
            {
                if (verInfo)
                {
                    u64Ver = verInfo->dwFileVersionMS;
                    u64Ver <<= 32;
                    u64Ver |= verInfo->dwFileVersionLS;
                }
            }
        }
        free(pData);
    }

    return u64Ver;
}

inline uint64_t VersionToUint64(const std::string &strVer)
{
    uint64_t u64Ver = 0;
    uint16_t* p = (uint16_t*)&u64Ver;
    char dot;
    std::istringstream iss(strVer);
    iss >> *(p + 3) >> dot >> *(p + 2) >> dot >> *(p + 1) >> dot >> *(p + 0) >> dot;
    return u64Ver;
}

inline uint64_t VersionToUint64(const std::wstring& strVer)
{
    uint64_t u64Ver = 0;
    uint16_t* p = (uint16_t*)&u64Ver;
    wchar_t dot;
    std::wistringstream iss(strVer);
    iss >> *(p + 3) >> dot >> *(p + 2) >> dot >> *(p + 1) >> dot >> *(p + 0) >> dot;
    return u64Ver;
}

inline std::string VersionToString(uint64_t u64Ver)
{
    uint16_t*wVer = (uint16_t*)&u64Ver;
    std::stringstream wss;
    wss << wVer[3] << "." << wVer[2] << "." << wVer[1] << "." << wVer[0];
    return wss.str();
}

inline std::wstring VersionToWString(uint64_t u64Ver)
{
    uint16_t* wVer = (uint16_t*)&u64Ver;
    std::wstringstream wss;
    wss << wVer[3] << "." << wVer[2] << "." << wVer[1] << "." << wVer[0];
    return wss.str();
}

template<typename StringT>
inline std::string GetFileVersionString(const StringT&lpFilePath)
{
    uint64_t u64Ver = GetFileVersion(lpFilePath);
    return VersionToString(u64Ver);
}
template<typename StringT>
inline std::wstring GetFileVersionWString(const StringT& lpFilePath)
{
    uint64_t u64Ver = GetFileVersion(lpFilePath);
    return VersionToWString(u64Ver);
}

} // namespace file
} // namespace sj

#endif //_FILE_VER_HPP_