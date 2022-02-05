#pragma once

#include "types.h"

/**
 * @brief Prints the help to the screen.
 */
void ShowHelp();

/**
 * @brief Add a new directory to be listed.
 *
 * @param arguments  pointer to the arguments data structure where path will be stored
 * @param path       the path to be stored
 */
void AddDirectoryToList(arguments_t *arguments, const char *path);

/**
 * @brief Remove the beginning white spaces of a string.
 *
 * @param s     pointer to a valid string
 * @return      pointer to the 's' parameter
 */
char *ltrim(char *s);

/**
 * @brief Remove the trailing white spaces of a string.
 *
 * @param s     pointer to a valid string
 * @return      pointer to the 's' parameter
 */
char *rtrim(char *s);

/**
 * @brief Remove white spaces from the beginning and
 * end of a string.
 *
 * @param s     pointer to a valid string
 * @return      pointer to the 's' parameter
 */
char *trim(char *s);

/**
 * @brief Check if string ends with a given suffix.
 *
 * @param str       string to check
 * @param suffix    suffix to search into the string
 * @return BOOL     TRUE if it ends with the suffix, FALSE otherwise
 */
BOOL StringEndsWith(const char *str, const char *suffix);

/**
 * @brief Given a filename, extract directory from filename.
 * By default it will return the filename if path can not be
 * extracted.
 *
 * @param path          name of the file to extract the directory
 * @param buffer        char array where data is stored
 * @param bufferSize    size in bytes of the buffer
 * @return const char*  pointer to the buffer array if directory extracted
 */
const char *GetDirectoryFromPath(const char *path, char *buffer, size_t bufferSize);

/**
 * @brief Get a human readable representation for the size,
 * if the size is zero a hyphen it will shown instead,
 * print sizes like 1K / 234M / 2G / etc.
 *
 * This function can not multi-threaded as it has a
 * static member variable to store the file size
 * as string and returns a pointer to it.
 *
 * @param bytes         size of the asset in bytes
 * @return const char*  human readable representation
 */
const char *GetFileSizeAsText(size_t bytes);

/**
 * @brief The current working directory. The directory has
 * a maximum length of 260 characters (MAX_PATH).
 *
 * @return const char* name of the directory.
 */
const char *GetWorkingDirectory();

/**
 * @brief Given a path it says it is current path, '.',
 * or parent path, '..'.
 *
 * @param path      relative path
 * @return BOOL     TRUE if it is '.' or '..', FALSE otherwise
 */
BOOL IsDotPath(const char *path);
