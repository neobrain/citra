// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#define BOOST_NO_SCOPED_ENUMS
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

#include <algorithm>

// TODO: Most of these includes should not be necessary anymore
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>        // for SHGetFolderPath
#include <shellapi.h>
#include <commdlg.h>    // for GetSaveFileName
#include <io.h>
#include <direct.h>        // getcwd
#endif

#if defined(__APPLE__)
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFURL.h>
#include <CoreFoundation/CFBundle.h>
#endif

#include "common/common.h"
#include "common/file_util.h"

namespace FileUtil
{

// Remove any ending forward slashes from directory paths
// Modifies argument.
static void StripTailDirSlashes(std::string &fname)
{
    if (fname.length() > 1)
    {
        size_t i = fname.length() - 1;
        while (fname[i] == DIR_SEP_CHR)
            fname[i--] = '\0';
    }
    return;
}

// Returns true if file filename exists
bool Exists(const std::string &filename)
{
    boost::system::error_code error;
    return boost::filesystem::exists(filename, error) && !error;
}

// Returns true if filename is a directory
bool IsDirectory(const std::string &filename)
{
    boost::system::error_code error;
    return boost::filesystem::is_directory(filename/*, error*/) && !error;
}

// Deletes a given filename, return true on success
bool Delete(const std::string &filename)
{
    INFO_LOG(COMMON, "Delete: file %s", filename.c_str());

    // We can't delete a directory
    if (IsDirectory(filename))
    {
        WARN_LOG(COMMON, "Delete failed: %s is a directory", filename.c_str());
        return false;
    }

    boost::system::error_code error;
    boost::filesystem::remove(filename, error);
    if (error) {
        ERROR_LOG(COMMON, "remove failed on %s: %s",
                 filename.c_str(), error.message().c_str());
        return false;
    }

    return true;
}

// Returns true if successful, or path already exists.
bool CreateDir(const std::string &path)
{
    INFO_LOG(COMMON, "CreateDir: directory %s", path.c_str());

    if (IsDirectory(path)) {
        // Job is already done
        return true;
    }

    boost::system::error_code error;
    if (!boost::filesystem::create_directory(path, error) || error) {
        ERROR_LOG(COMMON, "create_directory failed on %s: %i", path.c_str(), error.message().c_str());
        return false;
    }
    return true;
}

// Creates the full path of fullPath returns true on success
bool CreateFullPath(const std::string &fullPath)
{
    INFO_LOG(COMMON, "CreateFullPath: path %s", fullPath.c_str());

    if (IsDirectory(fullPath)) {
        // Job is already done
        return true;
    }

    boost::system::error_code error;
    if (!boost::filesystem::create_directories(fullPath, error) || error) {
        ERROR_LOG(COMMON, "create_directories failed on %s: %i",
                  fullPath.c_str(), error.message().c_str());
        return false;
    }
    return true;
}

// renames file srcFilename to destFilename, returns true on success
bool Rename(const std::string& srcFilename, const std::string& destFilename)
{
    INFO_LOG(COMMON, "Rename: %s --> %s",
            srcFilename.c_str(), destFilename.c_str());

    boost::system::error_code error;
    boost::filesystem::rename(srcFilename, destFilename, error);

    if (error) {
        ERROR_LOG(COMMON, "Rename: failed %s --> %s: %s",
                  srcFilename.c_str(), destFilename.c_str(), error.message().c_str());
        return false;
    }
    return true;
}

// copies file srcFilename to destFilename, returns true on success
bool Copy(const std::string& srcFilename, const std::string& destFilename)
{
    INFO_LOG(COMMON, "Copy: %s --> %s",
            srcFilename.c_str(), destFilename.c_str());

    boost::system::error_code error;
    boost::filesystem::copy(srcFilename, destFilename, error);

    if (error) {
        boost::filesystem::copy_file(srcFilename, destFilename,
                                     boost::filesystem::copy_option::overwrite_if_exists, error);
        if (error) {
            ERROR_LOG(COMMON, "Copy: failed %s --> %s: %s",
                srcFilename.c_str(), destFilename.c_str(), error.message().c_str());
            return false;
        }
    }
    return true;
}

// Returns the size of filename (64bit)
u64 GetSize(const std::string& filename)
{
    boost::system::error_code error;
    u64 ret = boost::filesystem::file_size(filename, error);

    if (error) {
        ERROR_LOG(COMMON, "file_size failed %s: %s",
                filename.c_str(), error.message().c_str());
        return 0;
    }

    return ret;
}

// Overloaded GetSize, accepts FILE*
u64 GetSize(FILE *f)
{
    // can't use off_t here because it can be 32-bit
    u64 pos = ftello(f);
    if (fseeko(f, 0, SEEK_END) != 0) {
        ERROR_LOG(COMMON, "GetSize: seek failed %p: %s",
              f, GetLastErrorMsg());
        return 0;
    }
    u64 size = ftello(f);
    if ((size != pos) && (fseeko(f, pos, SEEK_SET) != 0)) {
        ERROR_LOG(COMMON, "GetSize: seek failed %p: %s",
              f, GetLastErrorMsg());
        return 0;
    }
    return size;
}

// creates an empty file filename, returns true on success 
bool CreateEmptyFile(const std::string &filename)
{
    INFO_LOG(COMMON, "CreateEmptyFile: %s", filename.c_str()); 

    if (!FileUtil::IOFile(filename, "wb"))
    {
        ERROR_LOG(COMMON, "CreateEmptyFile: failed %s: %s",
                  filename.c_str(), GetLastErrorMsg());
        return false;
    }

    return true;
}


// Scans the directory tree gets, starting from _Directory and adds the
// results into parentEntry. Returns the number of files+directories found
u32 ScanDirectoryTree(const std::string &directory, FSTEntry& parentEntry)
{
    INFO_LOG(COMMON, "ScanDirectoryTree: directory %s", directory.c_str());

    if (!IsDirectory(directory))
        return 0;

    boost::system::error_code error;
    boost::filesystem::directory_iterator it(directory, error);
    boost::filesystem::directory_iterator end;

    // Iterate over all children and recurse on directories
    u32 foundEntries = 0;
    for (; it != end; ++it) {
        parentEntry.children.resize(1 + parentEntry.children.size());
        FSTEntry& entry = parentEntry.children.back();

        std::string filename = it->path().string();
        entry.virtualName = entry.physicalName = filename;
        if (IsDirectory(filename)) {
            entry.isDirectory = true;
            entry.size = ScanDirectoryTree(filename, entry);
            foundEntries += (u32)entry.size;
        } else {
            entry.isDirectory = false;
            entry.size = GetSize(filename);
        }

        ++foundEntries;
    }
    return foundEntries;
}


// Deletes the given directory and anything under it. Returns true on success.
// TODO: Rename to DeleteRecursively
bool DeleteDirRecursively(const std::string &directory)
{
    INFO_LOG(COMMON, "DeleteDirRecursively: %s", directory.c_str());

    if (!Exists(directory)) {
        // Return success because we only care about the directory not existing afterwards.
        return true;
    }

    boost::system::error_code error;
    boost::filesystem::remove_all(directory, error);

    if (error) {
        ERROR_LOG(COMMON, "remove_all failed on %s: %s",
                 directory.c_str(), error.message().c_str());
        return false;
    }
    return true;
}

// Create directory and copy contents (does not overwrite existing files)
// TODO: Rename to CopyRecursively
// TODO: Perform actual error checking here...
void CopyDir(const std::string &source_path, const std::string &dest_path)
{
    boost::system::error_code error;
    try {
        if (Exists(dest_path) && !IsDirectory(dest_path)) {
            // TODO: Error out
            ERROR_LOG(GPU, "Dest exists, but not a directory");
            return;
        }

        boost::filesystem::copy(source_path, dest_path, error);

        if (!IsDirectory(source_path)) {
            // Already done
            return;
        }

        boost::filesystem::directory_iterator it(source_path);
        boost::filesystem::directory_iterator end;

        for (; it != end; ++it) {
            std::string source = (it->path()).string();
            std::string dest   = (dest_path / it->path().filename()).string();
            if (IsDirectory(source)) {
                CopyDir(source, dest);
            } else {
                boost::filesystem::copy(source, dest, error);
            }
        }
    } catch (...) {
    }
}

// Returns the current directory
std::string GetCurrentDir()
{
    boost::system::error_code error;
    auto ret = boost::filesystem::current_path(error);
    if (error) {
        ERROR_LOG(COMMON, "GetCurrentDir failed: %s", error.message().c_str());
        return {};
    }
    return ret.string();
}

// Sets the current directory to the given directory
bool SetCurrentDir(const std::string &directory)
{
    return __chdir(directory.c_str()) == 0;
}

#if defined(__APPLE__)
std::string GetBundleDirectory() 
{
    CFURLRef BundleRef;
    char AppBundlePath[MAXPATHLEN];
    // Get the main bundle for the app
    BundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef BundlePath = CFURLCopyFileSystemPath(BundleRef, kCFURLPOSIXPathStyle);
    CFStringGetFileSystemRepresentation(BundlePath, AppBundlePath, sizeof(AppBundlePath));
    CFRelease(BundleRef);
    CFRelease(BundlePath);

    return AppBundlePath;
}
#endif

#ifdef _WIN32
std::string& GetExeDirectory()
{
    static std::string DolphinPath;
    if (DolphinPath.empty())
    {
        TCHAR Dolphin_exe_Path[2048];
        GetModuleFileName(NULL, Dolphin_exe_Path, 2048);
        DolphinPath = Common::TStrToUTF8(Dolphin_exe_Path);
        DolphinPath = DolphinPath.substr(0, DolphinPath.find_last_of('\\'));
    }
    return DolphinPath;
}
#endif

std::string GetSysDirectory()
{
    std::string sysDir;

#if defined (__APPLE__)
    sysDir = GetBundleDirectory();
    sysDir += DIR_SEP;
    sysDir += SYSDATA_DIR;
#else
    sysDir = SYSDATA_DIR;
#endif
    sysDir += DIR_SEP;

    INFO_LOG(COMMON, "GetSysDirectory: Setting to %s:", sysDir.c_str());
    return sysDir;
}

// Returns a string with a Dolphin data dir or file in the user's home
// directory. To be used in "multi-user" mode (that is, installed).
const std::string& GetUserPath(const unsigned int DirIDX, const std::string &newPath)
{
    static std::string paths[NUM_PATH_INDICES];

    // Set up all paths and files on the first run
    if (paths[D_USER_IDX].empty())
    {
#ifdef _WIN32
        paths[D_USER_IDX] = GetExeDirectory() + DIR_SEP USERDATA_DIR DIR_SEP;
#else
        if (FileUtil::Exists(ROOT_DIR DIR_SEP USERDATA_DIR))
            paths[D_USER_IDX] = ROOT_DIR DIR_SEP USERDATA_DIR DIR_SEP;
        else
            paths[D_USER_IDX] = std::string(getenv("HOME") ? 
                getenv("HOME") : getenv("PWD") ? 
                getenv("PWD") : "") + DIR_SEP EMU_DATA_DIR DIR_SEP;
#endif

        paths[D_CONFIG_IDX]            = paths[D_USER_IDX] + CONFIG_DIR DIR_SEP;
        paths[D_GAMECONFIG_IDX]        = paths[D_USER_IDX] + GAMECONFIG_DIR DIR_SEP;
        paths[D_MAPS_IDX]            = paths[D_USER_IDX] + MAPS_DIR DIR_SEP;
        paths[D_CACHE_IDX]            = paths[D_USER_IDX] + CACHE_DIR DIR_SEP;
        paths[D_SHADERCACHE_IDX]    = paths[D_USER_IDX] + SHADERCACHE_DIR DIR_SEP;
        paths[D_SHADERS_IDX]        = paths[D_USER_IDX] + SHADERS_DIR DIR_SEP;
        paths[D_STATESAVES_IDX]        = paths[D_USER_IDX] + STATESAVES_DIR DIR_SEP;
        paths[D_SCREENSHOTS_IDX]    = paths[D_USER_IDX] + SCREENSHOTS_DIR DIR_SEP;
        paths[D_DUMP_IDX]            = paths[D_USER_IDX] + DUMP_DIR DIR_SEP;
        paths[D_DUMPFRAMES_IDX]        = paths[D_DUMP_IDX] + DUMP_FRAMES_DIR DIR_SEP;
        paths[D_DUMPAUDIO_IDX]        = paths[D_DUMP_IDX] + DUMP_AUDIO_DIR DIR_SEP;
        paths[D_DUMPTEXTURES_IDX]    = paths[D_DUMP_IDX] + DUMP_TEXTURES_DIR DIR_SEP;
        paths[D_LOGS_IDX]            = paths[D_USER_IDX] + LOGS_DIR DIR_SEP;
        paths[F_DEBUGGERCONFIG_IDX]    = paths[D_CONFIG_IDX] + DEBUGGER_CONFIG;
        paths[F_LOGGERCONFIG_IDX]    = paths[D_CONFIG_IDX] + LOGGER_CONFIG;
        paths[F_MAINLOG_IDX]        = paths[D_LOGS_IDX] + MAIN_LOG;
    }

    if (!newPath.empty())
    {
        if (!FileUtil::IsDirectory(newPath))
        {
            WARN_LOG(COMMON, "Invalid path specified %s", newPath.c_str());
            return paths[DirIDX];
        }
        else
        {
            paths[DirIDX] = newPath;
        }

        switch (DirIDX)
        {
        case D_ROOT_IDX:
            paths[D_USER_IDX] = paths[D_ROOT_IDX] + DIR_SEP;
            paths[D_SYSCONF_IDX]    = paths[D_USER_IDX] + SYSCONF_DIR + DIR_SEP;
            paths[F_SYSCONF_IDX]    = paths[D_SYSCONF_IDX] + SYSCONF;
            break;

        case D_USER_IDX:
            paths[D_USER_IDX]        = paths[D_ROOT_IDX] + DIR_SEP;
            paths[D_CONFIG_IDX]            = paths[D_USER_IDX] + CONFIG_DIR DIR_SEP;
            paths[D_GAMECONFIG_IDX]        = paths[D_USER_IDX] + GAMECONFIG_DIR DIR_SEP;
            paths[D_MAPS_IDX]            = paths[D_USER_IDX] + MAPS_DIR DIR_SEP;
            paths[D_CACHE_IDX]            = paths[D_USER_IDX] + CACHE_DIR DIR_SEP;
            paths[D_SHADERCACHE_IDX]    = paths[D_USER_IDX] + SHADERCACHE_DIR DIR_SEP;
            paths[D_SHADERS_IDX]        = paths[D_USER_IDX] + SHADERS_DIR DIR_SEP;
            paths[D_STATESAVES_IDX]        = paths[D_USER_IDX] + STATESAVES_DIR DIR_SEP;
            paths[D_SCREENSHOTS_IDX]    = paths[D_USER_IDX] + SCREENSHOTS_DIR DIR_SEP;
            paths[D_DUMP_IDX]            = paths[D_USER_IDX] + DUMP_DIR DIR_SEP;
            paths[D_DUMPFRAMES_IDX]        = paths[D_DUMP_IDX] + DUMP_FRAMES_DIR DIR_SEP;
            paths[D_DUMPAUDIO_IDX]        = paths[D_DUMP_IDX] + DUMP_AUDIO_DIR DIR_SEP;
            paths[D_DUMPTEXTURES_IDX]    = paths[D_DUMP_IDX] + DUMP_TEXTURES_DIR DIR_SEP;
            paths[D_LOGS_IDX]            = paths[D_USER_IDX] + LOGS_DIR DIR_SEP;
            paths[D_SYSCONF_IDX]        = paths[D_USER_IDX] + SYSCONF_DIR DIR_SEP;
            paths[F_EMUCONFIG_IDX]        = paths[D_CONFIG_IDX] + EMU_CONFIG;
            paths[F_DEBUGGERCONFIG_IDX]    = paths[D_CONFIG_IDX] + DEBUGGER_CONFIG;
            paths[F_LOGGERCONFIG_IDX]    = paths[D_CONFIG_IDX] + LOGGER_CONFIG;
            paths[F_MAINLOG_IDX]        = paths[D_LOGS_IDX] + MAIN_LOG;
            break;

        case D_CONFIG_IDX:
            paths[F_EMUCONFIG_IDX]    = paths[D_CONFIG_IDX] + EMU_CONFIG;
            paths[F_DEBUGGERCONFIG_IDX]    = paths[D_CONFIG_IDX] + DEBUGGER_CONFIG;
            paths[F_LOGGERCONFIG_IDX]    = paths[D_CONFIG_IDX] + LOGGER_CONFIG;
            break;

        case D_DUMP_IDX:
            paths[D_DUMPFRAMES_IDX]        = paths[D_DUMP_IDX] + DUMP_FRAMES_DIR DIR_SEP;
            paths[D_DUMPAUDIO_IDX]        = paths[D_DUMP_IDX] + DUMP_AUDIO_DIR DIR_SEP;
            paths[D_DUMPTEXTURES_IDX]    = paths[D_DUMP_IDX] + DUMP_TEXTURES_DIR DIR_SEP;
            break;

        case D_LOGS_IDX:
            paths[F_MAINLOG_IDX]        = paths[D_LOGS_IDX] + MAIN_LOG;
        }
    }
    
    return paths[DirIDX];
}

//std::string GetThemeDir(const std::string& theme_name)
//{
//    std::string dir = FileUtil::GetUserPath(D_THEMES_IDX) + theme_name + "/";
//
//#if !defined(_WIN32)
//    // If theme does not exist in user's dir load from shared directory
//    if (!FileUtil::Exists(dir))
//        dir = SHARED_USER_DIR THEMES_DIR "/" + theme_name + "/";
//#endif
//    
//    return dir;
//}

bool WriteStringToFile(bool text_file, const std::string &str, const char *filename)
{
    return FileUtil::IOFile(filename, text_file ? "w" : "wb").WriteBytes(str.data(), str.size());
}

bool ReadFileToString(bool text_file, const char *filename, std::string &str)
{
    FileUtil::IOFile file(filename, text_file ? "r" : "rb");
    auto const f = file.GetHandle();

    if (!f)
        return false;

    str.resize(static_cast<u32>(GetSize(f)));
    return file.ReadArray(&str[0], str.size());
}

IOFile::IOFile()
    : m_file(NULL), m_good(true)
{}

IOFile::IOFile(std::FILE* file)
    : m_file(file), m_good(true)
{}

IOFile::IOFile(const std::string& filename, const char openmode[])
    : m_file(NULL), m_good(true)
{
    Open(filename, openmode);
}

IOFile::~IOFile()
{
    Close();
}

IOFile::IOFile(IOFile&& other)
    : m_file(NULL), m_good(true)
{
    Swap(other);
}

IOFile& IOFile::operator=(IOFile&& other)
{
    Swap(other);
    return *this;
}

void IOFile::Swap(IOFile& other)
{
    std::swap(m_file, other.m_file);
    std::swap(m_good, other.m_good);
}

bool IOFile::Open(const std::string& filename, const char openmode[])
{
    Close();
#ifdef _WIN32
    _tfopen_s(&m_file, Common::UTF8ToTStr(filename).c_str(), Common::UTF8ToTStr(openmode).c_str());
#else
    m_file = fopen(filename.c_str(), openmode);
#endif

    m_good = IsOpen();
    return m_good;
}

bool IOFile::Close()
{
    if (!IsOpen() || 0 != std::fclose(m_file))
        m_good = false;

    m_file = NULL;
    return m_good;
}

std::FILE* IOFile::ReleaseHandle()
{
    std::FILE* const ret = m_file;
    m_file = NULL;
    return ret;
}

void IOFile::SetHandle(std::FILE* file)
{
    Close();
    Clear();
    m_file = file;
}

u64 IOFile::GetSize()
{
    if (IsOpen())
        return FileUtil::GetSize(m_file);
    else
        return 0;
}

bool IOFile::Seek(s64 off, int origin)
{
    if (!IsOpen() || 0 != fseeko(m_file, off, origin))
        m_good = false;

    return m_good;
}

u64 IOFile::Tell()
{
    if (IsOpen())
        return ftello(m_file);
    else
        return -1;
}

bool IOFile::Flush()
{
    if (!IsOpen() || 0 != std::fflush(m_file))
        m_good = false;

    return m_good;
}

bool IOFile::Resize(u64 size)
{
    if (!IsOpen() || 0 !=
#ifdef _WIN32
        // ector: _chsize sucks, not 64-bit safe
        // F|RES: changed to _chsize_s. i think it is 64-bit safe
        _chsize_s(_fileno(m_file), size)
#else
        // TODO: handle 64bit and growing
        ftruncate(fileno(m_file), size)
#endif
    )
        m_good = false;

    return m_good;
}

} // namespace
