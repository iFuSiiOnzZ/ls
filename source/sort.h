#pragma once

#include "types.h"

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by directories fallowed by symlinks and any other type.
 *
 * @param lhv pointer to the first element to compare
 * @param rhv pointer to the second element to compare
 */
int OrderByDirectoryFirst(const void *lhv, const void *rhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the name.
 *
 * @param lhv pointer to the first element to compare
 * @param rhv pointer to the second element to compare
 */
int OrderByName(const void *lhv, const void *rhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the domain group.
 *
 * @param lhv pointer to the first element to compare
 * @param rhv pointer to the second element to compare
 */
int OrderByGroup(const void *lhv, const void *rhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the owner.
 *
 * @param lhv pointer to the first element to compare
 * @param rhv pointer to the second element to compare
 */
int OrderByOwner(const void *lhv, const void *rhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the size.
 *
 * @param lhv pointer to the first element to compare
 * @param rhv pointer to the second element to compare
 */
int OrderBySize(const void *lhv, const void *rhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the creation timestamp.
 *
 * @param lhv pointer to the first element to compare
 * @param rhv pointer to the second element to compare
 */
int OrderByCreationTimestamp(const void *lhv, const void *rhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the last time it was accessed.
 *
 * @param lhv pointer to the first element to compare
 * @param rhv pointer to the second element to compare
 */
int OrderByAccessedTimestamp(const void *lhv, const void *rhv);

/**
 * @brief Function used by 'qsort' algorithm to order the assets
 * by the last time it was modified.
 *
 * @param lhv pointer to the first element to compare
 * @param rhv pointer to the second element to compare
 */
int OrderByModifiedTimestamp(const void *lhv, const void *rhv);

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
