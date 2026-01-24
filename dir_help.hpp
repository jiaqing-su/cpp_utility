
#pragma once

#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <stdlib.h>
#include <string>
#include <functional>
#include <io.h>
#include <direct.h>
#include <CommonDefines.h>

#ifndef MAX_PATH
#define MAX_PATH _MAX_PATH
#endif // !MAX_PATH

//#include <shlobj_core.h>
//#include <Knownfolders.h>  // 包含 Known Folder ID 的定义
//#include <Objbase.h>       // 用于 CoTaskMemFree

//#pragma comment(lib, "shell32.lib")//SHGetKnownFolderPath
//#pragma comment(lib, "Ole32.lib") // 用于 CoTaskMemFree

namespace com
{
	namespace dir
	{
        static const class AppDataDir {
            std::wstring wstrAppDataPath;
            std::wstring wstrCurrentPath;
        public:
            AppDataDir() {

                if (wstrAppDataPath.empty()) {
					WCHAR userProfile[MAX_PATH];
					DWORD length = GetEnvironmentVariable(L"USERPROFILE", userProfile, MAX_PATH);
					if (length > 0) {
						wstrAppDataPath = userProfile;
						wstrAppDataPath += L"\\AppData\\Roaming";
					}
/*
					{
#if 1
						//SHGetSpecialFolderPath 不受支持,改用SHGetKnownFolderPath
						WCHAR tzAppDataPath[MAX_PATH] = { 0 };
						::SHGetSpecialFolderPathW(NULL, tzAppDataPath, CSIDL_APPDATA, FALSE);
						wstrAppDataPath = tzAppDataPath;
#else
						PWSTR szPath = NULL;
						SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &szPath);
						wstrAppDataPath = szPath;
						CoTaskMemFree(szPath);
#endif
					}
*/
                    wstrAppDataPath += L"\\";
                    wstrAppDataPath += _L(PRODUCT_NAME_EN);
                    wstrAppDataPath += L"\\";
                }
                if (wstrCurrentPath.empty()) {
                    WCHAR tzExe[MAX_PATH] = { 0 };
                    DWORD dwLen = 0;
                    dwLen = ::GetModuleFileNameW(NULL, tzExe, MAX_PATH);
                    if (dwLen >= 2)
                    {
                        while (tzExe[dwLen - 1] != L'\\')
                            dwLen--;
                        tzExe[dwLen - 1] = 0;
                    }
                    wstrCurrentPath = tzExe;
                }
            }
            operator std::wstring()const { return wstrAppDataPath; }
            inline std::wstring GetCurrentDir()const {
                return wstrCurrentPath;
            }
        }appDataDir;

		inline std::wstring GetCurrentDir()
		{
            return appDataDir.GetCurrentDir();
		}

		inline std::wstring GetResourceDir()
		{
            return (std::wstring)appDataDir + L"resource";
		}

		inline std::wstring GetResourceCfg()
		{
			std::wstring wstrFile = GetResourceDir();
			wstrFile += L"\\";
			wstrFile += _L(RES_JSON_FILE);
			return wstrFile;
		}

		inline std::wstring GetAppDataPath()
		{
            return (std::wstring)appDataDir;
		}

		inline std::wstring GetAppDataCamPath()
		{
			return (std::wstring)appDataDir + _L(PARTNER_NAME) + L"\\";
		}

		inline std::wstring GetAppDataUpdateServicePath()
		{
            return (std::wstring)appDataDir + L"UpdateService";
		}

		inline std::wstring GetCamConfigDir()
		{
            return (std::wstring)appDataDir + _L(PARTNER_NAME) + L"\\" + _L(PARTNER_NAME) + L".json";
		}
		
		inline std::wstring GetLiveFilepath()
		{
            return (std::wstring)appDataDir + _L(PARTNER_NAME) + L"\\" + _L(LIVE_JSON_FILE);
		}
		inline std::wstring GetCameraFilepath()
		{
			return (std::wstring)appDataDir + _L(PARTNER_NAME) + L"\\" + _L(CAMERA_JSON_FILE);
		}
		inline std::wstring GetGameFilepath()
		{
			return (std::wstring)appDataDir + _L(PARTNER_NAME) + L"\\" + _L(GAME_JSON_FILE);
		}
		inline std::wstring GetUpdateFilepath()
		{
            return (std::wstring)appDataDir + _L(PARTNER_NAME) + L"\\" + _L(UPDATE_JSON_FILE);
		}
		inline std::wstring GetLogDir() 
		{
            return (std::wstring)appDataDir + L"log";
		}
        inline std::wstring GetCefCacheDir()
        {
            return (std::wstring)appDataDir + L"cache";
        }
		inline std::wstring GetTempDir()
		{
			return (std::wstring)appDataDir + L"Temp";
		}
		inline void mkdir_p(const char *dir)
		{
			char path[MAX_PATH];
			char *cur = path;
			while (dir && *dir)
			{
				*cur = *dir;
				if (*cur == '\\' || *cur == '/')
				{
					*(cur + 1) = 0;
					if (_access(path, 0) != 0)
					{
						if (_mkdir(path))
							break;
					}
				}
				cur++;
				dir++;
			}
		}

		inline void mkdir_p(const wchar_t *dir)
		{
			wchar_t path[MAX_PATH];
			wchar_t *cur = path;
			while (dir && *dir)
			{
				*cur = *dir;
				if (*cur == L'\\' || *cur == L'/')
				{
					*(cur + 1) = 0;
					if (_waccess(path, 0) != 0)
					{
						if (_wmkdir(path))
							break;
					}
				}
				cur++;
				dir++;
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<typename CharT>
		struct FindTraits {};

		template<>
		struct FindTraits<wchar_t>
		{
			using FindData = _wfinddata_t;
			static constexpr const wchar_t* dot = L".";
			static constexpr const wchar_t* dotdot = L"..";
			static constexpr const wchar_t* pattern = L"*.*";
			static constexpr intptr_t(*FindFirst)(const wchar_t*, _wfinddata_t*) = _wfindfirst;
			static constexpr int(*FindNext)(intptr_t, _wfinddata_t*) = _wfindnext;
			static constexpr int(*FindClose)(intptr_t) = _findclose;
		};

		template<>
		struct FindTraits<char>
		{
			using FindData = _finddata_t;
			static constexpr const char* dot = ".";
			static constexpr const char* dotdot = "..";
			static constexpr const char* pattern = "*.*";
			static constexpr intptr_t(*FindFirst)(const char*, _finddata_t*) = _findfirst;
			static constexpr int(*FindNext)(intptr_t, _finddata_t*) = _findnext;
			static constexpr int(*FindClose)(intptr_t) = _findclose;
		};

		template<typename CharT>
		inline void list_files(const std::basic_string<CharT>& folder, bool recursive, std::function<void(unsigned int, const CharT*)> callback)
		{
			using Traits = FindTraits<CharT>;

			auto path = folder;
			if (path.empty()) {
				path += Traits::dot;
				path += CharT('/');
			}
			else if (*path.crbegin() != CharT('\\') && *path.crbegin() != CharT('/'))
				path += CharT('/');
			//path += "*.*";

			typename Traits::FindData finddata;
			intptr_t handle = Traits::FindFirst((path + Traits::pattern).c_str(), &finddata);
			if (handle != -1)
			{
				do
				{
					const CharT* name = finddata.name;
					if (std::char_traits<CharT>::compare(name, Traits::dot, 1) != 0 &&
						std::char_traits<CharT>::compare(name, Traits::dotdot, 2) != 0)
					{
						if (callback)
							callback(finddata.attrib, (path + name).c_str());
						if (recursive && (finddata.attrib & _A_SUBDIR))
							list_files<CharT>(path + name, recursive, callback);
					}
					memset(&finddata, 0, sizeof(finddata));
				} while (Traits::FindNext(handle, &finddata) == 0);
				Traits::FindClose(handle);
			}
		}

		template<typename CharT>
		inline void list_files(const CharT* folder, bool recursive, std::function<void(unsigned int, const CharT*)> callback) {
			list_files<CharT>(std::basic_string<CharT>(folder), recursive, callback);
		}

		/*
		* flag=1,file;flag=2,folder;other,all
		*/
		template<typename CharT>
		inline std::vector<std::basic_string<CharT>> list_files(const std::basic_string<CharT>& folder_path, bool recursive = true, unsigned int flag = 1) {
			std::vector<std::basic_string<CharT>> files;
			list_files<CharT>(folder_path, recursive, [&](unsigned int attrib, const CharT* name) {
#ifdef _WIN32
				bool is_directory = (attrib & _A_SUBDIR) != 0; // FILE_ATTRIBUTE_DIRECTORY
#else
				// 对于 Linux/macOS，假设 attrib 是 struct stat 的 st_mode
				bool is_directory = (attrib & S_IFMT) == S_IFDIR;
#endif
				if (flag == 1 && is_directory || flag == 2 && !is_directory)
					return;
				files.push_back(name);
				});
			return files;
		}
		template<typename CharT>
		inline std::vector<std::basic_string<CharT>> list_files(const CharT* folder_path, bool recursive = true, unsigned int flag = 1) {
			return list_files(std::basic_string<CharT>(folder_path), recursive, flag);
		}

		inline int remove(const std::string& file) {
			return remove(file.c_str());
		}
		inline int remove(const std::wstring& file) {
			return _wremove(file.c_str());
		}

		template<typename CharT>
		inline void remove_dir(const std::basic_string<CharT>& folder) {
			using Traits = FindTraits<CharT>;

			auto path = folder;
			if (path.empty()) {
				path += Traits::dot;
				path += CharT('/');
			}
			else if (*path.crbegin() != CharT('\\') && *path.crbegin() != CharT('/'))
				path += CharT('/');
			//path += "*.*";

			typename Traits::FindData finddata;
			intptr_t handle = Traits::FindFirst((path + Traits::pattern).c_str(), &finddata);
			if (handle != -1)
			{
				do
				{
					const CharT* name = finddata.name;
					if (std::char_traits<CharT>::compare(name, Traits::dot, 1) != 0 &&
						std::char_traits<CharT>::compare(name, Traits::dotdot, 2) != 0)
					{
						auto filepath = path + CharT('\\') + name;
						bool is_directory = (finddata.attrib & _A_SUBDIR) == 0;
						if (is_directory) {
							remove_dir(filepath);
						}
						remove(filepath);
					}
					memset(&finddata, 0, sizeof(finddata));
				} while (Traits::FindNext(handle, &finddata) == 0);
				Traits::FindClose(handle);
			}
		}
		template<typename CharT>
		inline void remove_dir(const CharT* folder) {
			std::basic_string<CharT> path(folder);
			remove_dir(path);
		}

        inline bool exists(const std::wstring& path)
        {
            return _waccess(path.c_str(), 0) == 0;
        }
        inline bool exists(const std::string& path)
        {
            return _access(path.c_str(), 0) == 0;
        }


	} // namespace dir
} // namespace com