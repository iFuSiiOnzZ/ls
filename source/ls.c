
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _DEBUG
#   define _CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#endif

#include "directory.h"
#include "win32.h"

#include "utils.h"
#include "sort.h"

///////////////////////////////////////////////////////////////////////////////

static const char VERSION[] = "0.0.1 "__DATE__" "__TIME__;

///////////////////////////////////////////////////////////////////////////////

// Check if pointer is not NULL, free it and assign NULL to it
#define CHECK_DELETE(x) do { if(x) { free(x); x = NULL; } } while(0)

// Check if handle is not NULL, close it and assign NULL to it
#define CHECK_CLOSE_HANDLE(x) do { if(x) { CloseHandle(x); x = NULL; } } while(0)

// Compute the size of compile time array
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

///////////////////////////////////////////////////////////////////////////////

static BOOL  g_PrintWithColor = FALSE;  // Colorize the output or not

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
 * @return ETextColor   the color used fpr printing
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

/**
 * @brief Parse short arguments
 * ex: -l, -a, -laR, ...
 *
 * @param arg           string with the arguments, each letter is a argument
 * @param arguments     pointer to arguments data structure where data is stored
 */
static void ParseShortArgument(const char *arg, arguments_t *arguments)
{
    for (const char *c = arg; *c != '\0'; ++c)
    {
        switch (*c)
        {
            case 'A': arguments->showAlmostAll = arguments->showAll = TRUE; break;
            case 'R': arguments->recursiveList = TRUE; break;
            case 'l': arguments->showMetaData = TRUE; break;
            case 'r': arguments->reverseOrder = TRUE; break;
            case 'v': arguments->showVersion = TRUE; break;
            case '?': arguments->showHelp = TRUE; break;
            case 'a': arguments->showAll = TRUE; break;
        }
    }
}

/**
 * @brief Parse long arguments.
 * ex: --icons, --colors, --group-directories-first, ...
 *
 * @param arg           double pointer to the string with the argument
 * @param arguments     pointer to argument structure where data is stored
 * @return              pointer to the next argument
 */
static const char **ParseLongArgument(const char **arg, arguments_t *arguments)
{
    if (strcmp(*arg, "--group-directories-first") == 0)
    {
        arguments->sortField = SORT_DIRECTORY_FIRST;
    }
    else if (strcmp(*arg, "--almost-all") == 0)
    {
        arguments->showAlmostAll = TRUE;
        arguments->showAll = TRUE;
    }
    else if (strcmp(*arg, "--recursive") == 0)
    {
        arguments->recursiveList = TRUE;
    }
    else if (strcmp(*arg, "--version") == 0)
    {
        arguments->showVersion = TRUE;
    }
    else if (strcmp(*arg, "--virterm") == 0)
    {
        arguments->virtualTerminal = TRUE;
    }
    else if (strcmp(*arg, "--reverse") == 0)
    {
        arguments->reverseOrder = TRUE;
    }
    else if (strcmp(*arg, "--long") == 0)
    {
        arguments->showMetaData = TRUE;
    }
    else if (strcmp(*arg, "--help") == 0)
    {
        arguments->showHelp = TRUE;
    }
    else if (strcmp(*arg, "--colors") == 0)
    {
        arguments->colors = TRUE;
    }
    else if (strcmp(*arg, "--icons") == 0)
    {
        arguments->showIcons = TRUE;
    }
    else if (strcmp(*arg, "--all") == 0)
    {
        arguments->showAll = TRUE;
    }
    else if (strcmp(*arg, "--sai") == 0)
    {
        arguments->showIcons = TRUE;
        arguments->showAvailableIcons = TRUE;
    }
    else if (strcmp(*arg, "--sort") == 0)
    {
        ++arg;
        if (_strcmpi(*arg, "NAME") == 0)
        {
            arguments->sortField = SORT_BY_NAME;
        }
        else if (_strcmpi(*arg, "SIZE") == 0)
        {
            arguments->sortField = SORT_BY_SIZE;
        }
        else if (_strcmpi(*arg, "OWNER") == 0)
        {
            arguments->sortField = SORT_BY_OWNER;
        }
        else if (_strcmpi(*arg, "GROUP") == 0)
        {
            arguments->sortField = SORT_BY_GROUP;
        }
        else if (_strcmpi(*arg, "CREATED") == 0)
        {
            arguments->sortField = SORT_BY_CREATION_DATE;
        }
        else if (_strcmpi(*arg, "ACCESSED") == 0)
        {
            arguments->sortField = SORT_BY_LAST_ACCESSED;
        }
        else if (_strcmpi(*arg, "MODIFIED") == 0)
        {
            arguments->sortField = SORT_BY_LAST_MODIFIED;
        }
        else
        {
            printf_s("Invalid sort argument: %s\n", *arg);
            printf_s("Valid fields are: NAME, SIZE, OWNER, GROUP, CREATED, ACCESSED, MODIFIED (insensitive case)");
            exit(1);
        }
    }

    return arg;
}

/**
 * @brief Parse program arguments.
 * ex: -l, -a, -la, --color, ...
 *
 * If no directory is detected it will get the current
 * directory (".\") (full path).
 *
 * @param argc          number of arguments
 * @param argv          pointer of char array, the arguments
 * @return arguments_t the data structure of the arguments parsed
 */
static arguments_t ParseArguments(int argc, const char *argv[])
{
    arguments_t retData = { 0 };
    const char **currentArg = argv + 1;
    const char **lastArg = argv + argc;

    while (currentArg < lastArg)
    {
        if ((*currentArg)[0] == '-' && (*currentArg)[1] == '-')
        {
            currentArg = ParseLongArgument(currentArg, &retData);
        }
        else if ((*currentArg)[0] == '-')
        {
            ParseShortArgument((*currentArg) + 1, &retData);
        }
        else
        {
            AddDirectoryToList(&retData, (*currentArg));
        }

        ++currentArg;
    }

    return retData;
}

/**
 * @brief Prints to screen the assets found.
 *
 * @param content       pointer to the directory containing the assets
 * @param arguments     pointer to the parsed arguments structure
 */
static void PrintAssetInformation(const directory_t *content, const char *directoryName, arguments_t *arguments)
{
    char currentPath[MAX_PATH] = { 0 };
    GetDirectoryFromPath(directoryName, currentPath, MAX_PATH);

    size_t ownerLength = 0, domainLength = 0;
    size_t directoryLength = strlen(currentPath);

    for (size_t i = 0; i < content->size && arguments->showMetaData; ++i)
    {
        size_t s = strlen(content->data[i].domain);
        domainLength = domainLength < s ? s : domainLength;

        s = strlen(content->data[i].owner);
        ownerLength = ownerLength < s ? s : ownerLength;
    }

    for (size_t i = 0; i < content->size; ++i)
    {
        if (!arguments->showMetaData)
        {
            text_color_t textColor = GetTextNameColor(&content->data[i]);
            color_printf(textColor, "%s\n", content->data[i].name);
            continue;
        }

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
        textColor = BLUE;
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

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    #if defined(WIN32) && defined(_DEBUG)
    {
        int nOldState = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        _CrtSetDbgFlag(nOldState | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
    }
    #endif

    arguments_t arguments = ParseArguments(argc, argv);

    if (arguments.showIcons && !SetConsoleOutputCP(65001))
    {
        printf_s("WARNING:\n");
        printf_s("Can not set console to UNICODE-UTF8. Some characters may not display correctly.\n\n");
    }

    if (arguments.virtualTerminal && (arguments.virtualTerminal = EnableVirtualTerminal()) == FALSE)
    {
        printf_s("WARNING:\n");
        printf_s("Can not enable virtual terminal.\n\n");
    }

    if (arguments.showAvailableIcons)
    {
        for (size_t i = 0; i < ARRAY_SIZE(g_AssetMetaData); ++i)
        {
            printf_s("% 30s  %s\n", g_AssetMetaData[i].ext, g_AssetMetaData[i].icon);
        }

        return EXIT_SUCCESS;
    }

    if (arguments.showHelp)
    {
        ShowHelp();
        return EXIT_SUCCESS;
    }

    if (arguments.showVersion)
    {
        printf_s("%s", VERSION);
        return EXIT_SUCCESS;
    }

    if (arguments.currentDir == NULL)
    {
        AddDirectoryToList(&arguments, GetWorkingDirectory());
    }

    g_PrintWithColor = arguments.colors;
    BOOL extraDirs = arguments.currentDir->next != NULL || arguments.recursiveList;

    while (arguments.currentDir != NULL)
    {
        directory_list_t *dir = arguments.currentDir;
        directory_t *directory = GetDirectoryContent(dir->path, &arguments);

        if (directory == NULL || dir == NULL)
        {
            printf_s("\"%s\": No such file or directory\n", dir->path);
            goto next_dir;
        }

        if (directory->size == 0)
        {
            goto next_dir;
        }

        SortDirectoryContent(directory, &arguments);
        if (extraDirs) printf_s("%s\n", dir->path);

        PrintAssetInformation(directory, dir->path, &arguments);
        if (arguments.currentDir->next != NULL) printf_s("\n\n");

        next_dir:
        arguments.currentDir = arguments.currentDir->next;
        CHECK_DELETE(directory);
        free(dir);
    }

    if (arguments.virtualTerminal)
    {
        DisableVirtualTerminal();
    }

    return EXIT_SUCCESS;
}
