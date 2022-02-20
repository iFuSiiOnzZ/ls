
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

#include "screen.h"

///////////////////////////////////////////////////////////////////////////////

static const char VERSION[] = "0.0.1 "__DATE__" "__TIME__;

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
            case 'l': arguments->showLongFormat = TRUE; break;
            case 'R': arguments->recursiveList = TRUE; break;
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
        arguments->showLongFormat = TRUE;
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
    else if (strcmp(*arg, "--smd") == 0)
    {
        arguments->showIcons = TRUE;
        arguments->showMetaData = TRUE;
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

    if (arguments.showMetaData)
    {
        ShowMetaData(&arguments);
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

    while (arguments.currentDir != NULL)
    {
        directory_list_t *dir = arguments.currentDir;
        directory_t *directory = GetDirectoryContent(dir->path, &arguments);

        if (directory == NULL)
        {
            printf_s("\"%s\": No such file or directory\n", dir->path);
            goto next_dir;
        }

        if (directory->size == 0)
        {
            goto next_dir;
        }

        SortDirectoryContent(directory, &arguments);
        if (dir->next) printf_s("%s\n", dir->path);

        if (arguments.showLongFormat)
        {
            PrintAssetLongFormat(directory, dir->path, &arguments);
            if (arguments.currentDir->next != NULL) printf_s("\n\n");
        }
        else
        {
            PrintAssetShortFormat(directory, &arguments);
            if (arguments.currentDir->next != NULL) printf_s("\n\n");
        }

    next_dir:
        arguments.currentDir = arguments.currentDir->next;
        CHECK_DELETE(directory);
        CHECK_DELETE(dir);
    }

    if (arguments.virtualTerminal)
    {
        DisableVirtualTerminal();
    }

    return EXIT_SUCCESS;
}
