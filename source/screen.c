#include "screen.h"
#include "utils.h"

#include <stdio.h>

// Compute the size of compile time array
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// Colorize the output or not
static BOOL g_PrintWithColor = FALSE;

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
 * @brief Prints text on the screen with a given color. If the global variable
 * 'g_PrintWithColor' is not set the text it will be printed using the default
 * console text color.
 *
 * @param textColor text color, see 'ETextColor' (types.h)
 * @param fmt       format of the text or the text itself
 * @param ...       variable arguments list
 */
static void color_printf(text_color_t textColor, const char *fmt, ...)
{
    if (!g_PrintWithColor)
    {
        va_list args = NULL;
        va_start(args, fmt);

        vprintf_s(fmt, args);
        va_end(args);

        return;
    }

    WORD wOldColorAttrs = 0;
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo = { 0 };

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbiInfo);

    wOldColorAttrs = csbiInfo.wAttributes;
    SetConsoleTextAttribute(hConsole, textColor);

    va_list args = NULL;
    va_start(args, fmt);

    vprintf_s(fmt, args);
    va_end(args);

    SetConsoleTextAttribute(hConsole, wOldColorAttrs);
}

/**
 * @brief Prints text on the screen with a given color. If the global variable
 * 'g_PrintWithColor' is not set the text it will be printed using the default
 * console text color.
 *
 * It makes use of the Virtual Console sequence
 * https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
 *
 * @param r         red channel intensity (0 to 255)
 * @param g         green channel intensity (0 to 255)
 * @param b         blue channel intensity (0 to 255)
 * @param fmt       format of the text or the text itself
 * @param ...       variable arguments list
 */
static void color_printf_vt(int r, int g, int b, const char *fmt, ...)
{
    if (!g_PrintWithColor)
    {
        va_list args = NULL;
        va_start(args, fmt);

        vprintf_s(fmt, args);
        va_end(args);

        return;
    }

    char print_fmt[1024];
    const char *color_fmt = "\x1b[38;2;%d;%d;%dm%s\033[0m";
    sprintf_s(print_fmt, sizeof(print_fmt), color_fmt, r, g, b, fmt);

    va_list args = NULL;
    va_start(args, fmt);

    vprintf_s(print_fmt, args);
    va_end(args);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Given an asset with its properties return the color used for
 * printing based on the asset type.
 *
 * Default color is WHITE. Directories will be print as GREEN.
 * Encrypted files as BLUE, compress files as MAGENTA, temporary
 * as DARKGREY, system files as RED and symbolic kink as CYAN.
 *
 * @param asset         pointer to the asset (file or directory)
 * @return ETextColor   the color used for printing
 */
static text_color_t GetTextNameColor(const asset_t *asset)
{
    text_color_t textColor = WHITE;

    if (asset->type.symlink)
    {
        textColor = CYAN;
    }
    else if (asset->type.directory)
    {
        textColor = GREEN;
    }
    else if (asset->type.compressed)
    {
        textColor = MAGENTA;
    }
    else if (asset->type.encrypted)
    {
        textColor = BLUE;
    }
    else if (asset->type.temporary)
    {
        textColor = DARKGRAY;
    }
    else if (asset->type.system)
    {
        textColor = RED;
    }

    return textColor;
}

/**
 * @brief Given an asset, return char with its representation.
 * It tells us if is a directory, symbolic link or other type.
 *
 * 'd' for directory
 * 'l' for symbolic links
 * '-' for any other format
 *
 * @param data      pointer to the asset data structure
 * @return char     'd', 'l' or '-'
 */
static char GetContentType(const asset_t *data)
{
    // NOTE(Andrei): Order dependency, a symlink can also have
    //               the directory attribute marked.

    if (data->type.symlink)   return 'l';
    if (data->type.directory) return 'd';

    return '-';
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
    char name[MAX_PATH];
    strcpy_s(name, MAX_PATH, data->name);

    for (size_t i = 0; i < MAX_PATH && name[i] != '\0'; ++i)
    {
        name[i] = (char)tolower(name[i]);
    }

    for (size_t i = 0; i < ARRAY_SIZE(g_AssetMetaData); ++i)
    {
        if (StringEndsWith(name, g_AssetMetaData[i].ext))
        {
            return &g_AssetMetaData[i];
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

///////////////////////////////////////////////////////////////////////////////

void PrintAssetLongFormat(const directory_t *content, const char *directoryName, const arguments_t *arguments)
{
    g_PrintWithColor = arguments->colors;

    char currentPath[MAX_PATH] = { 0 };
    GetDirectoryFromPath(directoryName, currentPath, MAX_PATH);

    size_t ownerLength = 0, domainLength = 0;
    size_t directoryLength = strlen(currentPath);

    for (size_t i = 0; i < content->size && arguments->showLongFormat; ++i)
    {
        size_t s = strlen(content->data[i].domain);
        domainLength = domainLength < s ? s : domainLength;

        s = strlen(content->data[i].owner);
        ownerLength = ownerLength < s ? s : ownerLength;
    }

    for (size_t i = 0; i < content->size; ++i)
    {
        // Content type
        text_color_t textColor = GRAY;
        color_printf(textColor, "%c", GetContentType(&content->data[i]));

        // Permission read
        textColor = YELLOW;
        color_printf(textColor, "%c", content->data[i].accessRights.read ? 'r' : '-');

        // Permission write
        textColor = RED;
        color_printf(textColor, "%c", content->data[i].accessRights.write ? 'w' : '-');

        // Permission execution
        textColor = GREEN;
        color_printf(textColor, "%c", content->data[i].accessRights.execution ? 'x' : '-');

        // File size
        textColor = GREEN;
        color_printf(textColor, "%s  ", GetFileSizeAsText(content->data[i].size));

        // Domain
        textColor = YELLOW;
        color_printf(textColor, "%*.*s  ", domainLength, domainLength, content->data[i].domain);

        // Owner
        textColor = DARKYELLOW;
        color_printf(textColor, "%*.*s  ", ownerLength, ownerLength, content->data[i].owner);

        // Creation date
        textColor = CYAN;
        color_printf(textColor, "%s  ", content->data[i].date);

        // File name
        textColor = GetTextNameColor(&content->data[i]);
        const asset_metadata_t *assetMetadata = GetAssetMetadata(&content->data[i]);

        if (arguments->showIcons)
        {
            if (arguments->virtualTerminal)
            {
                color_printf_vt(assetMetadata->r, assetMetadata->g, assetMetadata->b, "%s ", assetMetadata->icon);
            }
            else
            {
                color_printf(textColor, "%s ", assetMetadata->icon);
            }
        }

        if (arguments->recursiveList)
        {
            if (arguments->virtualTerminal)
            {
                color_printf_vt(assetMetadata->r, assetMetadata->g, assetMetadata->b, "%s", &content->data[i].path[directoryLength + 1]);
            }
            else
            {
                color_printf(textColor, "%s", &content->data[i].path[directoryLength + 1]);
            }
        }
        else
        {
            if (arguments->virtualTerminal)
            {
                color_printf_vt(assetMetadata->r, assetMetadata->g, assetMetadata->b, "%s", content->data[i].name);
            }
            else
            {
                color_printf(textColor, "%s", content->data[i].name);
            }
        }

        // Show where symlink is pointing
        if (content->data[i].link[0] != '\0')
        {
            printf_s(" -> ");
            color_printf(textColor, "%s", content->data[i].link);
        }

        if (i < content->size - 1)
        {
            putchar('\n');
        }
    }
}

void PrintAssetShortFormat(const directory_t *content, const arguments_t *arguments)
{
    g_PrintWithColor = arguments->colors;

    for (size_t i = 0; i < content->size; ++i)
    {
        text_color_t textColor = GetTextNameColor(&content->data[i]);
        const asset_metadata_t *assetMetadata = GetAssetMetadata(&content->data[i]);

        if (arguments->showIcons)
        {
            if (arguments->virtualTerminal)
            {
                color_printf_vt(assetMetadata->r, assetMetadata->g, assetMetadata->b, "%s ", assetMetadata->icon);
            }
            else
            {
                color_printf(textColor, "%s ", assetMetadata->icon);
            }
        }

        if (arguments->virtualTerminal)
        {
            color_printf_vt(assetMetadata->r, assetMetadata->g, assetMetadata->b, "%s\n", content->data[i].name);
        }
        else
        {
            color_printf(textColor, "%s\n", content->data[i].name);
        }
    }
}

void ShowMetaData(const arguments_t *arguments)
{
    g_PrintWithColor = arguments->colors;

    for (size_t i = 0; i < ARRAY_SIZE(g_AssetMetaData); ++i)
    {
        const char fmt[] = "(%3d, %3d, %3d)  %s  %s\n";
        const asset_metadata_t *m = &g_AssetMetaData[i];

        if (arguments->virtualTerminal)
        {
            color_printf_vt(m->r, m->g, m->b, fmt, m->r, m->g, m->b, m->icon, m->ext);
        }
        else
        {
            printf_s(fmt, m->r, m->g, m->b, m->icon, m->ext);
        }
    }
}
