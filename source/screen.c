#include "screen.h"
#include "utils.h"
#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum of 2 values
#define MIN(x, y) ((x) < (y) ? (x) : (y))

// Minimum of 2 values
#define MAX(x, y) ((x) > (y) ? (x) : (y))

// Clamp a value between a low and a high
#define CLAMP(v, l, h) MIN(h, MAX(l, v))

// Colorize the output or not
static BOOL g_PrintWithColor = FALSE;

// Maximum number of columns
#define MAX_NUM_COLS 64

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Some predefined colors.
 */
typedef enum text_color_t
{
    BLACK = 0,
    DARKBLUE = FOREGROUND_BLUE,
    DARKGREEN = FOREGROUND_GREEN,
    DARKCYAN = FOREGROUND_GREEN | FOREGROUND_BLUE,
    DARKRED = FOREGROUND_RED,
    DARKMAGENTA = FOREGROUND_RED | FOREGROUND_BLUE,
    DARKYELLOW = FOREGROUND_RED | FOREGROUND_GREEN,
    DARKGRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    GRAY = FOREGROUND_INTENSITY,
    BLUE = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    GREEN = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    CYAN = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
    RED = FOREGROUND_INTENSITY | FOREGROUND_RED,
    MAGENTA = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
    YELLOW = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    WHITE = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
} text_color_t;

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Column information.
 *
 * 'size'   : number of character needed for the column
 */
typedef struct col_t
{
    size_t size;
} col_t;

/**
 * @brief Row information.
 *
 * 'size'   : number of columns in the row
 * 'cols'   : information of each column (maximun of 'MAX_NUM_COLS')
 */
typedef struct row_t
{
    size_t size;
    col_t cols[MAX_NUM_COLS];
} row_t;

///////////////////////////////////////////////////////////////////////////////

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
    return;
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

    return;
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
 * @brief Get the number of columns to display the content as grid.
 *
 * @param content   pointer to the directory containing the assets
 * @param showIcons take into account the icons?
 * @return row_t    number of columns
 */
static row_t GetNumberOfColumns(const directory_t *content, BOOL showIcons, size_t padding)
{
    size_t totalSize = 0;
    size_t width = 0, height = 0;

    GetScreenBufferSize(&width, &height);
    size_t *textSizeArray = (size_t*)malloc(sizeof(size_t) * content->size);

    for (size_t i = 0; i < content->size; ++i)
    {
        size_t s = strlen(content->data[i].name) + padding;
        s += showIcons ? strlen(content->data[i].metadata->icon) : 0;

        totalSize += s;
        textSizeArray[i] = s;
    }

    size_t avgColSize = totalSize / content->size;
    size_t numCols = width / avgColSize, maxSize = 0;

    row_t ret = { 0 };
    ret.size = CLAMP(numCols, 1,  MAX_NUM_COLS);

compute_column_size:
    maxSize = 0;
    memset(ret.cols, 0, sizeof(size_t) * ret.size);

    for (size_t i = 0; i < content->size; ++i)
    {
        col_t *c = &ret.cols[i % ret.size];
        c->size = MAX(textSizeArray[i], c->size);
    }

    for (size_t i = 0; i < ret.size; ++i)
    {
        maxSize += ret.cols[i].size;
    }

    if (maxSize > width && ret.size > 1)
    {
        --ret.size;
        goto compute_column_size;
    }

    // Note(Andrei): Remove padding from last column
    ret.cols[ret.size - 1].size -= padding;

    CHECK_DELETE(textSizeArray);
    return ret;
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
        const asset_metadata_t *m = content->data[i].metadata;

        if (arguments->showIcons)
        {
            if (arguments->virtualTerminal)
            {
                color_printf_vt(m->r, m->g, m->b, "%s ", m->icon);
            }
            else
            {
                color_printf(textColor, "%s ", m->icon);
            }
        }

        if (arguments->recursiveList)
        {
            if (arguments->virtualTerminal)
            {
                color_printf_vt(m->r, m->g, m->b, "%s", &content->data[i].path[directoryLength + 1]);
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
                color_printf_vt(m->r, m->g, m->b, "%s", content->data[i].name);
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
    BOOL showIcons = arguments->showIcons;

    size_t extraspace = showIcons ? 3 : 2;
    row_t row = GetNumberOfColumns(content, arguments->showIcons, extraspace);

    for (size_t i = 0; i < content->size; ++i)
    {
        size_t ri = i % row.size;
        if (i > 0 && ri == 0) putchar('\n');

        text_color_t textColor = GetTextNameColor(&content->data[i]);
        const asset_metadata_t *m = content->data[i].metadata;

        if (arguments->showIcons)
        {
            if (arguments->virtualTerminal)
            {
                color_printf_vt(m->r, m->g, m->b, "%s ", m->icon);
            }
            else
            {
                color_printf(textColor, "%s ", m->icon);
            }
        }

        if (arguments->virtualTerminal)
        {
            color_printf_vt(m->r, m->g, m->b, "%s", content->data[i].name);
        }
        else
        {
            color_printf(textColor, "%s", content->data[i].name);
        }

        size_t s = strlen(content->data[i].name);
        s += showIcons ? strlen(m->icon) + 1 : 0;

        if (row.size > 1 && s < row.cols[ri].size)
        {
            int padLen = (int)(row.cols[ri].size - s);
            printf_s("%*.*s", padLen, padLen, " ");
        }
    }
}

void ShowMetaData(const arguments_t *arguments)
{
    g_PrintWithColor = arguments->colors;

    for (size_t i = 0; i < ARRAY_SIZE(g_AssetFullNameMetaData); ++i)
    {
        const char fmt[] = "(%3d, %3d, %3d)  %s  %s\n";
        const asset_metadata_t *m = &g_AssetFullNameMetaData[i];

        if (arguments->virtualTerminal)
        {
            color_printf_vt(m->r, m->g, m->b, fmt, m->r, m->g, m->b, m->icon, m->ext);
        }
        else
        {
            printf_s(fmt, m->r, m->g, m->b, m->icon, m->ext);
        }
    }

    for (size_t i = 0; i < ARRAY_SIZE(g_AssetExtensionMetaData); ++i)
    {
        const char fmt[] = "(%3d, %3d, %3d)  %s  %s\n";
        const asset_metadata_t *m = &g_AssetExtensionMetaData[i];

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
