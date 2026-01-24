#ifndef _FILE_HELP_HPP_
#define _FILE_HELP_HPP_

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <io.h>
#include <direct.h>
#define mkdir(p) _mkdir(p)
#define access _access
//#define close _close
#else
#include <unistd.h> //io.h
#include <dirent.h> //opendir
#define mkdir(p) mkdir(p, 0755)
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <functional>
#include <string>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

namespace sj
{
namespace file
{

//#define exists(path) (access(path, 0) == 0)
#define DEFAULT_PATH_SLASH '\\'

inline bool is_slash(int c)
{
    return (c == '/') || (c == '\\');
}

inline bool is_exists(const std::string &filepath)
{
    return (_access(filepath.c_str(), 0) == 0);
}

inline bool is_exists(const std::wstring &filepath)
{
    return (_waccess(filepath.c_str(), 0) == 0);
}

inline int get_stat(const std::string &filepath, struct _stat64 *st)
{
    return _stat64(filepath.c_str(), st);
}

inline int get_stat(const std::wstring &filepath, struct _stat64 *st)
{
    return _wstat64(filepath.c_str(), st);
}

inline int make_dir(const std::string &filepath)
{
    return _mkdir(filepath.c_str());
}

inline int make_dir(const std::wstring &filepath)
{
    return _wmkdir(filepath.c_str());
}

template <typename _StringType>
inline bool is_dir(const _StringType &filepath)
{
    struct _stat64 st;
    if (0 == get_stat(filepath.c_str(), &st))
    {
        if (st.st_mode & S_IFDIR)
            return true;
    }
    return false;
}

template <typename _StringType>
inline bool is_file(const _StringType &filepath)
{
    struct _stat64 st;
    if (0 == get_stat(filepath.c_str(), &st))
    {
        if (st.st_mode & S_IFREG)
            return true;
    }
    return false;
}

template <typename _StringType>
inline const _StringType make_standard_filepath(const _StringType &filepath)
{
    _StringType path = filepath;
    if (path.empty() || !is_slash(*path.rbegin()))
        path += _StringType(DEFAULT_PATH_SLASH);
    return path;
}

template <typename _StringType>
inline const _StringType get_filename(const _StringType &filepath)
{
    auto filename = filepath;
    int i = filepath.length() - 1;
    for (; i >= 0; i--)
    {
        if (is_slash(filepath[i]))
        {
            filename = filepath.substr(i + 1);
            break;
        }
    }
    return filename;
}

inline bool get_file_time(const std::string &filepath, time_t *create_time, time_t *last_access_time, time_t *last_modify_time)
{
    bool bGet = false;
    struct stat buf;
    int fd = 0;
    _sopen_s(&fd, filepath.c_str(), _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    //int fd = _open(filepath.c_str(), _O_RDONLY);
    if (fd >= 0)
    {
        if (0 == fstat(fd, &buf))
        {
            if (create_time)
                *create_time = buf.st_ctime;
            if (last_access_time)
                *last_access_time = buf.st_atime;
            if (last_modify_time)
                *last_modify_time = buf.st_mtime;
            bGet = true;
        }
        _close(fd);
    }
    return bGet;
}

template <typename _StringType>
inline int mkdir_p(const _StringType &filepath)
{
    _StringType path;
    for (const auto &c : filepath)
    {
        path.push_back(c);
        if (is_slash(c))
        {
            if (!is_exists(path))
            {
                if (make_dir(path))
                    return -1;
            }
        }
    }

    if (path.length() < filepath.length())
    {
        if (!is_exists(filepath))
        {
            if (make_dir(filepath))
                return -1;
        }
    }

    return 0;
}

template <typename _CharType>
inline int mkdir_p(const _CharType *filepath)
{
    using _StringType = std::basic_string<_CharType>;
    _StringType path = filepath;
    return mkdir_p(path);
}

inline void list_files(const std::string &folder, std::function<void(bool is_dir, const std::string &name)> callback, bool recursive)
{
    if (is_file(folder))
    {
        if (callback)
            callback(false, folder);
        return;
    }

    auto base_dir = folder;
    if (base_dir.empty() || *base_dir.rbegin() != '\\' && *base_dir.rbegin() != '/')
        base_dir += "\\";

#if defined(_WIN32)
    auto search_dir = base_dir + "*.*";
    struct _finddata_t finddata;
    intptr_t handle = _findfirst(search_dir.c_str(), &finddata);
    if (handle != -1)
    {
        do
        {
            const char *name = finddata.name;
            unsigned int attrib = finddata.attrib;
            bool is_dir = ((attrib & _A_SUBDIR) != 0);

            if (strcmp(name, ".") && strcmp(name, ".."))
            {
                std::string filepath = base_dir + name;

                if (is_dir)
                    filepath += "\\";

                if (callback)
                    callback(is_dir, filepath);

                if (is_dir && recursive)
                    list_files(filepath, callback, recursive);
            }

            memset(&finddata, 0, sizeof(finddata));
        } while (_findnext(handle, &finddata) == 0);
        _findclose(handle);
    }
#else
    struct dirent *pdirent;
    DIR *d_info = opendir(base_dir.c_str());
    if (d_info)
    {
        while ((pdirent = readdir(d_info)) != NULL)
        {
            const char *name = pdirent->d_name;
            unsigned int attrib = pdirent->d_type;
            bool is_dir = ((attrib & DT_DIR) != 0);

            if (strcmp(name, ".") && strcmp(name, ".."))
            {
                auto filepath = base_dir + name;

                if (is_dir)
                    filepath += "/";

                if (callback)
                    callback(is_dir, filepath);

                if (is_dir && recursive)
                    list_files(filepath, callback, recursive);
            }
        }
        closedir(d_info);
    }
#endif
}

inline void remove_files(const std::string &filepath)
{
    std::string cmd = is_dir(filepath) ? "rmdir" : "del";
    cmd += " /S /Q ";
    cmd += filepath;
    system(cmd.c_str());
}

} // namespace file
} // namespace sj

#endif // !_FILE_HELP_HPP_
