#pragma once

#include "types.h"

/**
 * @brief Get the asset permission for the current user. It will check for
 * READ, WRITE and EXECUTION permissions. By default all permissions will
 * be set to FALSE.
 *
 * @param path   full path of the asset
 * @param asset  pointer of the asset data structure where information is stored
 */
void GetPermissions(const char *path, asset_t *asset);

/**
 * @brief Get the owner and the owner domain of the asset.
 * By default an hyphen it will be show.
 *
 * @param path      full path of the asset
 * @param asset     pointer of the asset data structure where information is stored
 * @return BOOL     TRUE if owner and domain can be retrieved, FALSE otherwise
 */
BOOL GetOwnerAndDomain(const char *path, asset_t *asset);

/**
 * @brief Get symbolic link real path.
 *
 * @param path      full path of the symbolic link
 * @param asset     pointer of the asset data structure where information is stored
 * @return BOOL     TRUE is path can be retrieved, FALSE otherwise
 */
BOOL GetLinkTarget(const char *path, asset_t *asset);

/**
 * @brief Translate the Win32 attributes to the asset data types.
 *
 * @param attributes    win32 asset attributes
 * @param asset         pointer of the asset data structure where information is stored
 */
void TranslateAttributes(size_t attributes, asset_t *asset);

/**
 * @brief Given a path it says is the valid directory or not.
 *
 * @param path  full path
 * @return BOOL TRUE is valid, FALSE otherwise
 */
BOOL IsValidDirectory(const char *path);

/**
 * @brief Given a path it says is the valid document or not.
 *
 * @param path  full path
 * @return BOOL TRUE is valid, FALSE otherwise
 */
BOOL IsValidDocument(const char *path);

/**
 * @brief Translate the Win32 file size format to bytes size.
 *
 * By default size is split
 * in 2 parts, low and high. (high * (MAXDWORD+1)) + low
 *
 * @param fd        pointer to a valid WIN32_FIND_DATAA data structure
 * @return size_t   the size of the asset in bytes
 */
size_t TranslateFileSize(WIN32_FIND_DATAA *fd);

/**
 * @brief Enable virtual terminal.
 * https://docs.microsoft.com/es-es/windows/console/console-virtual-terminal-sequences
 */
BOOL EnableVirtualTerminal();

/**
 * @brief Disable virtual terminal.
 * https://docs.microsoft.com/es-es/windows/console/console-virtual-terminal-sequences
 */
BOOL DisableVirtualTerminal();

/**
 * @brief Get terminal screen buffer size.
 *
 * @param width     valid pointer where the width is stored
 * @param height    valid pointer where the height is stored
 * @return BOOL     TRUE if it can be retrieved, FALSE otherwise
 */
BOOL GetScreenBufferSize(int *width, int *height);

/**
 * @brief Get the terminal cursor position.
 *
 * @param x     valid pointer where 'x' coordinate is stored
 * @param y     valid pointer where 'y' coordinate is stored
 * @return BOOL TRUE if it can be retrieved, FALSE otherwise
 */
BOOL GetCursorPosition(int *x, int *y);

/**
 * @brief Set the terminal cursor position.
 *
 * @param x     cursor 'x' coordinate
 * @param y     cursor 'y' coordinate
 * @return BOOL TRUE if it can be set, FALSE otherwise
 */
BOOL SetCursorPosition(int x, int y);
