#include "sort.h"
#include "types.h"

#include <stdlib.h>
#include <string.h>

local_function char GetContentType(const asset_t *data)
{
    if (data->type.symlink)     return 'l';
    if (data->type.directory)   return 'd';

    return 'z';
}

int OrderByDirectoryFirst(const void *lhv, const void *rhv)
{
    const asset_t *a = lhv; const asset_t *b = rhv;
    return GetContentType(a) - GetContentType(b);
}

int OrderByName(const void *lhv, const void *rhv)
{
    const asset_t *a = lhv; const asset_t *b = rhv;
    return _strcmpi(a->name, b->name);
}

int OrderByGroup(const void *lhv, const void *rhv)
{
    const asset_t *a = lhv; const asset_t *b = rhv;
    return _strcmpi(a->domain, b->domain);
}

int OrderByOwner(const void *lhv, const void *rhv)
{
    const asset_t *a = lhv; const asset_t *b = rhv;
    return _strcmpi(a->owner, b->owner);
}

int OrderBySize(const void *lhv, const void *rhv)
{
    const asset_t *a = lhv; const asset_t *b = rhv;
    return (int)((long long)b->size - (long long)a->size);
}

int OrderByCreationTimestamp(const void *lhv, const void *rhv)
{
    const asset_t *a = lhv; const asset_t *b = rhv;
    return (int)((long long)b->timestamp.creation - (long long)a->timestamp.creation);
}

int OrderByAccessedTimestamp(const void *lhv, const void *rhv)
{
    const asset_t *a = lhv; const asset_t *b = rhv;
    return (int)((long long)b->timestamp.access - (long long)a->timestamp.access);
}

int OrderByModifiedTimestamp(const void *lhv, const void *rhv)
{
    const asset_t *a = lhv; const asset_t *b = rhv;
    return (int)((long long)b->timestamp.modification - (long long)a->timestamp.modification);
}

void ReverseOrder(directory_t *directory)
{
    for (size_t low = 0, high = directory->size - 1; low < high; low++, high--)
    {
        asset_t temp = directory->data[low];
        directory->data[low] = directory->data[high];
        directory->data[high] = temp;
    }
}

void SortDirectoryContent(directory_t *directory, const arguments_t *arguments)
{
    switch (arguments->sortField)
    {
        case SORT_DIRECTORY_FIRST: qsort(directory->data, directory->size, sizeof(asset_t), OrderByDirectoryFirst); break;

        case SORT_BY_NAME: qsort(directory->data, directory->size, sizeof(asset_t), OrderByName); break;
        case SORT_BY_SIZE: qsort(directory->data, directory->size, sizeof(asset_t), OrderBySize); break;

        case SORT_BY_OWNER: qsort(directory->data, directory->size, sizeof(asset_t), OrderByOwner); break;
        case SORT_BY_GROUP: qsort(directory->data, directory->size, sizeof(asset_t), OrderByGroup); break;

        case SORT_BY_CREATION_DATE: qsort(directory->data, directory->size, sizeof(asset_t), OrderByCreationTimestamp); break;
        case SORT_BY_LAST_MODIFIED: qsort(directory->data, directory->size, sizeof(asset_t), OrderByModifiedTimestamp); break;
        case SORT_BY_LAST_ACCESSED: qsort(directory->data, directory->size, sizeof(asset_t), OrderByAccessedTimestamp); break;
    }

    if (arguments->reverseOrder)
    {
        ReverseOrder(directory);
    }
}
