#pragma once

#include "types.h"

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by directories fallowed by symlinks and any other type.
 *
 * @param a pointer to the first element to compare
 * @param b pointer to the second element to compare
 */
int OrderByDirectoryFirst(const asset_t *a, const asset_t *b);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the name.
 *
 * @param a pointer to the first element to compare
 * @param b pointer to the second element to compare
 */
int OrderByName(const asset_t *a, const asset_t *b);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the domain group.
 *
 * @param a pointer to the first element to compare
 * @param b pointer to the second element to compare
 */
int OrderByGroup(const asset_t *a, const asset_t *b);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the owner.
 *
 * @param a pointer to the first element to compare
 * @param b pointer to the second element to compare
 */
int OrderByOwner(const asset_t *a, const asset_t *b);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the size.
 *
 * @param a pointer to the first element to compare
 * @param b pointer to the second element to compare
 */
int OrderBySize(const asset_t *a, const asset_t *b);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the creation timestamp.
 *
 * @param a pointer to the first element to compare
 * @param b pointer to the second element to compare
 */
int OrderByCreationTimestamp(const asset_t *a, const asset_t *b);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the last time it was accessed.
 *
 * @param a pointer to the first element to compare
 * @param b pointer to the second element to compare
 */
int OrderByAccessedTimestamp(const asset_t *a, const asset_t *b);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the last time it was modified.
 *
 * @param a pointer to the first element to compare
 * @param b pointer to the second element to compare
 */
int OrderByModifiedTimestamp(const asset_t *a, const asset_t *b);

/**
 * @brief Reverse the assets order of a directory.
 *
 * @param directory pointer to the content
 */
void ReverseOrder(directory_t *directory);

/**
 * @brief Give a directory with its content, sort it by the sort field
 * given inside the arguments data structure.
 *
 * @param directory pointer to the directory with the assets to sort
 * @param arguments pointer to data structure with the sort arguments
 */
void SortDirectoryContent(directory_t *directory, const arguments_t *arguments);
