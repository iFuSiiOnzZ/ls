#pragma once

#include "types.h"

/**
 * @brief Get the assets inside a given path. Wild cards can be used
 * for the asset name, the wild card is represented by '*'.
 * ex: C:\Windows\System32\*.dll
 *
 * @param directory         pointer to the directory path
 * @param arguments         pointer to the parsed arguments structure
 * @return directory_t*     container with the assets information or NULL otherwise
 */
directory_t *GetDirectoryContent(const char *path, arguments_t *arguments);
