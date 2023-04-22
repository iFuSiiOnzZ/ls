#pragma once

#include "types.h"

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by directories fallowed by symlinks and any other type.
 *
 * @param rhv pointer to the first element to compare
 * @param lhv pointer to the second element to compare
 */
int OrderByDirectoryFirst(const void *rhv, const void *lhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the name.
 *
 * @param rhv pointer to the first element to compare
 * @param lhv pointer to the second element to compare
 */
int OrderByName(const void *rhv, const void *lhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the domain group.
 *
 * @param rhv pointer to the first element to compare
 * @param lhv pointer to the second element to compare
 */
int OrderByGroup(const void *rhv, const void *lhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the owner.
 *
 * @param rhv pointer to the first element to compare
 * @param lhv pointer to the second element to compare
 */
int OrderByOwner(const void *rhv, const void *lhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the size.
 *
 * @param rhv pointer to the first element to compare
 * @param lhv pointer to the second element to compare
 */
int OrderBySize(const void *rhv, const void *lhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the creation timestamp.
 *
 * @param rhv pointer to the first element to compare
 * @param lhv pointer to the second element to compare
 */
int OrderByCreationTimestamp(const void *rhv, const void *lhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the last time it was accessed.
 *
 * @param rhv pointer to the first element to compare
 * @param lhv pointer to the second element to compare
 */
int OrderByAccessedTimestamp(const void *rhv, const void *lhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the last time it was modified.
 *
 * @param rhv pointer to the first element to compare
 * @param lhv pointer to the second element to compare
 */
int OrderByModifiedTimestamp(const void *rhv, const void *lhv);

/**
 * @brief Reverse the assets order of rhv directory.
 *
 * @param directory pointer to the content
 */
void ReverseOrder(directory_t *directory);

/**
 * @brief Give rhv directory with its content, sort it by the sort field
 * given inside the arguments data structure.
 *
 * @param directory pointer to the directory with the assets to sort
 * @param arguments pointer to data structure with the sort arguments
 */
void SortDirectoryContent(directory_t *directory, const arguments_t *arguments);
