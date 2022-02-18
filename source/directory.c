#include "directory.h"
#include "win32.h"
#include "utils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

// Check if the file/directory attribute is marked as hidden
#define IS_HIDDEN(x)        ((x) & FILE_ATTRIBUTE_HIDDEN)

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Given a path it says it is current path, '.',
 * or parent path, '..'.
 *
 * @param path      relative path
 * @return BOOL     TRUE if it is '.' or '..', FALSE otherwise
 */
static BOOL IsDotPath(const char *path)
{
    return strcmp(path, ".") == 0 || strcmp(path, "..") == 0;
}

/**
 * @brief It tels if the attribute is marked as HIDDEN or
 * if the name it starts with dot (hidden files on Linux).
 *
 * @param attributes    asset attributes
 * @param name          asset name
 * @return BOOL        TRUE if it hidden or dot, FALSE otherwise
 */
static BOOL IsHiddenOrDot(size_t attributes, const char *name)
{
    return IS_HIDDEN(attributes) || name[0] == '.' || name[0] == '$';
}

/**
 * @brief Check if string ends with a given suffix.
 *
 * @param str       string to check
 * @param suffix    suffix to search into the string
 * @return BOOL     TRUE if it ends with the suffix, FALSE otherwise
 */
static BOOL StringEndsWith(const char *str, const char *suffix)
{
    if (!str || !suffix) return FALSE;

    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);

    if (lensuffix > lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

/**
 * @brief Convert a FILETIME timestap to a human representation.  By default
 * the creation time it will be used, is case of sorting it will be used to
 * sort timestap required.
 *
 * ex: 01 Jan 10:00 or 01 Jan 2022
 *
 * If the current year and the year of the FILETIME is not the same it will
 * print the FILETIME year, otherwise the hour and minutes of the FILETIME.
 *
 * @param fd        pointer to Win32 data structure to access the timestapms
 * @param arguments arguments data structure to know which timestamp to use
 * @param asset     pointer to the asset of date is stored
 * @return BOOL     TRUE if can be converted, FALSE otherwise
 */
static BOOL GetTimestaps(const WIN32_FIND_DATAA *fd, const arguments_t *arguments, asset_t *asset)
{
    memset(asset->date, 0, DATE_SIZE);
    ULARGE_INTEGER ul = { 0 };

    SYSTEMTIME systemTime = { 0 };
    SYSTEMTIME currentSystemTime = { 0 };

    FileTimeToSystemTime(&fd->ftCreationTime, &systemTime);
    GetSystemTime(&currentSystemTime);

    ul.HighPart = fd->ftCreationTime.dwHighDateTime;
    ul.LowPart = fd->ftCreationTime.dwHighDateTime;
    asset->timestamp.creation = ul.QuadPart;

    ul.HighPart = fd->ftLastAccessTime.dwHighDateTime;
    ul.LowPart = fd->ftLastAccessTime.dwHighDateTime;
    asset->timestamp.access = ul.QuadPart;

    ul.HighPart = fd->ftLastWriteTime.dwHighDateTime;
    ul.LowPart = fd->ftLastWriteTime.dwHighDateTime;
    asset->timestamp.modification = ul.QuadPart;

    switch (arguments->sortField)
    {
        case SORT_BY_LAST_ACCESSED: FileTimeToSystemTime(&fd->ftLastAccessTime, &systemTime); break;
        case SORT_BY_LAST_MODIFIED: FileTimeToSystemTime(&fd->ftLastWriteTime, &systemTime); break;
    }

    // NOTE(Andrei): SystemTime month starts with 1
    //               so add padding value for 0.
    static const char *m[13] =
    {
        "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    if (currentSystemTime.wYear != systemTime.wYear)
    {
        return sprintf_s
        (
            asset->date, DATE_SIZE,
            "%02d %s  %d",

            systemTime.wDay, m[systemTime.wMonth],
            systemTime.wYear
        ) == S_OK;
    }

    return sprintf_s
    (
        asset->date, DATE_SIZE,
        "%02d %s %02d:%02d",

        systemTime.wDay, m[systemTime.wMonth],
        systemTime.wHour, systemTime.wMinute
    ) == S_OK;
}

/**
 * @brief Helper function to resize the asset array.  The capacity it is
 * increased by half of the previous one.
 *
 * @param container pointer to the directory container
 * @return *directory_t pointer to the directory
 */
static directory_t *ResizeAssetArray(directory_t **container)
{
    if (container == NULL || (*container) == NULL)
    {
        size_t containerSize = sizeof(directory_t) + sizeof(asset_t) * STARTUP_CONTAINER_SIZE;
        directory_t *retData = malloc(containerSize);

        if (retData == NULL) { return FALSE; }
        if (container != NULL) (*container) = retData;

        retData->capacity = STARTUP_CONTAINER_SIZE;
        retData->size = 0;

        memset(retData, 0, sizeof(containerSize));
        return retData;
    }

    if ((*container)->size < (*container)->capacity)
    {
        return (*container);
    }

    size_t newCapacity = (size_t)((*container)->capacity + (*container)->capacity * 0.5f + 0.5f);
    size_t containerSize = sizeof(directory_t) + sizeof(asset_t) * newCapacity;

    directory_t *newRetData = realloc(*container, containerSize);
    if (newRetData == NULL) { return FALSE; }

    size_t memSetSize = (newCapacity - newRetData->size) * sizeof(asset_t);
    memset(&newRetData->data[newRetData->size], 0, memSetSize);

    newRetData->capacity = newCapacity;
    (*container) = newRetData;

    return (*container);
}

/**
 * @brief Get the metadata based on the asset extension. If the extension is not
 * found a predefined metadata based on the type it will be returned.
 *
 * @param data                      valid pointer to the asset
 * @return const asset_metadata_t*  pointer to the metadata structure
 */
static const asset_metadata_t *GetAssetMetadata(const asset_t *data)
{
    char name[MAX_PATH] = { 0 };
    strcpy_s(name, MAX_PATH, data->name);

    for (size_t i = 0; i < MAX_PATH && name[i] != '\0'; ++i)
    {
        name[i] = (char)tolower(name[i]);
    }

    for (size_t i = 0; i < ARRAY_SIZE(g_AssetFullNameMetaData); ++i)
    {
        if (strcmp(name, g_AssetFullNameMetaData[i].ext) == 0)
        {
            return &g_AssetFullNameMetaData[i];
        }
    }

    for (size_t i = 0; i < ARRAY_SIZE(g_AssetExtensionMetaData); ++i)
    {
        if (StringEndsWith(name, g_AssetExtensionMetaData[i].ext))
        {
            return &g_AssetExtensionMetaData[i];
        }
    }

    // Symlink and directory
    if (data->type.symlink && data->type.directory)
    {
        static asset_metadata_t symlinkdir = { 139, 233, 253, "", u8"\uf482" };
        return &symlinkdir;
    }

    // Symlink and file
    if (data->type.symlink)
    {
        static asset_metadata_t symlink = { 139, 233, 253, "", u8"\uf481" };
        return &symlink;
    }

    // Directory
    if (data->type.directory)
    {
        static asset_metadata_t dir = { 80, 250, 123, "", u8"\uf74a" };
        return &dir;
    }

    // Any other type
    static asset_metadata_t oth = { 255, 255, 255, "", u8"\uf15b" };
    return &oth;
}

/**
 * @brief Get the assets inside a given path. Wild cards can be used
 * for the asset name, the wild card is represented by '*'.
 * ex: C:\Windows\System32\*.dll
 *
 * @param directory         pointer to the directory path
 * @param arguments         pointer to the parsed arguments structure
 * @return directory_t*     container with the assets information or NULL otherwise
 */
directory_t *GetDirectoryContent(const char *path, arguments_t *arguments)
{
    char temporalPath[MAX_PATH] = { 0 };
    WIN32_FIND_DATAA fd = { 0 };

    if (strstr(path, "*") || IsValidDocument(path))
    {
        snprintf(temporalPath, sizeof(temporalPath), "%s", path);
    }
    else if (IsValidDirectory(path))
    {
        snprintf(temporalPath, sizeof(temporalPath), "%s%s", path, "\\*");
    }
    else
    {
        return NULL;
    }

    HANDLE hFind = FindFirstFileExA(temporalPath, FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE) return NULL;

    directory_t *retData = NULL;
    char currentPath[MAX_PATH] = { 0 };
    GetDirectoryFromPath(path, currentPath, MAX_PATH);

    do
    {
        if (ResizeAssetArray(&retData) == NULL)
        {
            FindClose(hFind);
            return retData;
        }

        if (arguments->showAlmostAll && IsDotPath(fd.cFileName))
        {
            continue;
        }

        if (!arguments->showAll && IsHiddenOrDot(fd.dwFileAttributes, fd.cFileName))
        {
            continue;
        }

        size_t index = retData->size++;
        asset_t *asset = &retData->data[index];

        memset(asset, 0, sizeof(asset_t));
        snprintf(temporalPath, sizeof(temporalPath), "%s\\%s", currentPath, fd.cFileName);

        strcpy_s(asset->name, PATH_SIZE, fd.cFileName);
        strcpy_s(asset->path, PATH_SIZE, temporalPath);

        GetPermissions(temporalPath, asset);
        GetOwnerAndDomain(temporalPath, asset);

        GetTimestaps(&fd, arguments, asset);
        TranslateAttributes(fd.dwFileAttributes, asset);

        asset->size = TranslateFileSize(&fd);
        asset->metadata = GetAssetMetadata(asset);

        if (asset->type.symlink)
        {
            GetLinkTarget(temporalPath, asset);
        }

        if (arguments->recursiveList && asset->type.directory && !IsDotPath(fd.cFileName))
        {
            AddDirectoryToList(arguments, temporalPath);
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
    return retData;
}
