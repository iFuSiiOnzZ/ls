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
 * @brief Convert a FILETIME timestap to a human representation.
 * By default the creation time it will be used, is case of
 * sorting it will be used to sort timestap required.
 * ex: 01 Jan 10:00 or 01 Jan 2022
 *
 * If the current year and the year of the FILETIME is not the
 * same it will print the FILETIME year, otherwise the hour
 * and minutes of the FILETIME.
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
 * @brief Helper function to resize the asset array.
 * The capacity it is increased by half of the
 * previous one.
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
    if (path == NULL)
    {
        return NULL;
    }

    char assetPath[MAX_PATH] = { 0 };
    WIN32_FIND_DATAA fd = { 0 };

    if (strstr(path, "*") || IsValidDocument(path))
    {
        snprintf(assetPath, sizeof(assetPath), "%s", path);
    }
    else if (IsValidDirectory(path))
    {
        snprintf(assetPath, sizeof(assetPath), "%s%s", path, "\\*");
    }
    else
    {
        return NULL;
    }

    HANDLE hFind = FindFirstFileExA(assetPath, FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0);
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

        size_t assetIndex = retData->size++;
        asset_t *asset = &retData->data[assetIndex];

        memset(asset, 0, sizeof(asset_t));
        snprintf(assetPath, sizeof(assetPath), "%s\\%s", currentPath, fd.cFileName);

        strcpy_s(asset->name, PATH_SIZE, fd.cFileName);
        strcpy_s(asset->path, PATH_SIZE, assetPath);

        GetPermissions(assetPath, asset);
        GetOwnerAndDomain(assetPath, asset);

        GetTimestaps(&fd, arguments, asset);
        asset->size = TranslateFileSize(&fd);
        TranslateAttributes(fd.dwFileAttributes, asset);

        if (asset->type.symlink)
        {
            GetLinkTarget(assetPath, asset);
        }

        if (arguments->recursiveList && asset->type.directory && !IsDotPath(fd.cFileName))
        {
            AddDirectoryToList(arguments, assetPath);
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
    return retData;
}
