#include "utils.h"
#include "types.h"
#include "win32.h"

#include <stdio.h>
#include <stdlib.h>


/**
 * @brief Given a string and a list of delimiters, find the where in the string
 * it is the last characted defined by delimiters.  The delimiters is also a
 * string where each character of the string is a delimiter.
 *
 * @param str           pointer to a valid string
 * @param delimiters    pointer to a valid list of delimiters
 * @return const char*  pointer to the character or NULL if not found.
 */
const char *FindLastDelimiter(const char *str, const char *delimiters)
{
    if (str == NULL || delimiters == NULL) return NULL;
    size_t len = strlen(str);

    if (len < 1)
    {
        return NULL;
    }

    for (const char *c = str + len - 1; c >= str; --c)
    {
        const char d[2] = { *c, '\0' };
        if (strstr(delimiters, d)) return c;
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

void ShowHelp()
{
    {
        const char *help =
            "Usage\n"
            "  ls.exe [options] [files...]\n\n"

            "  -?, --help                       show list of command-line options\n"
            "  -v, --version                    show version of ls\n\n";

        printf_s("%s", help);
    }

    {
        const char *help =
            "DISPLAY OPTION\n"
            "  -l, --long                       display extended file metadata as a table\n"
            "  -R, --recursive                  recurse into directories\n"
            "      --icons                      show icons associated to file/folder\n"
            "      --colors                     colorize the output\n"
            "      --virterm                    use virtual terminal for better colors\n\n";

        printf_s("%s", help);
    }

    {
        const char *help =
            "FILTERING AND SORTING OPTIONS\n"
            "  -a, --all                        show all file (include hidden and 'dot' files)\n"
            "  -A, --almost-all                 show all files avoiding '.' and '..'\n"
            "  -r, --reverse                    reverse the sort order\n"
            "      --sort [FIELD]               which field to sort by\n"
            "      --group-directories-first    list directories before other files\n\n";

        printf_s("%s", help);
    }

    {
        const char *help =
            "TIPS\n"
            "  pattern      Specifies the search pattern for the files to match\n"
            "               Wildcards * and ? can be used in the pattern.\n"
            "               ex: ls -l C:\\Windows\\System32\\*.dll\n\n"

            "  sort         Valid fields are: NAME, SIZE, OWNER, GROUP,\n"
            "               CREATED, ACCESSED and MODIFIED.\n"
            "               Fields are insensitive case.\n\n"

            "  icons        To be able to see the icons correctly you have to use the NerdFonts\n"
            "               https://github.com/ryanoasis/nerd-fonts\n"
            "               https://www.nerdfonts.com/";

        printf_s("%s", help);
    }
}

void AddDirectoryToList(arguments_t *arguments, const char *path)
{
    directory_list_t *dir = malloc(sizeof(directory_list_t));
    if (dir == NULL) return;

    strncpy_s(dir->path, MAX_PATH, path, MAX_PATH);
    dir->next = NULL;

    if (arguments->tailDir == NULL)
    {
        arguments->tailDir = dir;
        arguments->headDir = arguments->tailDir;
    }
    else
    {
        arguments->tailDir->next = dir;
        arguments->tailDir = dir;
    }
}

const char *GetDirectoryFromPath(const char *path, char *buffer, size_t bufferSize)
{
    sprintf_s(buffer, bufferSize, "%s", path);

    if (IsValidDocument(path) || strstr(buffer, "*"))
    {
        char *c = (char *)FindLastDelimiter(buffer, "\\/");

        if (c == NULL) strncpy_s(buffer, bufferSize, GetWorkingDirectory(), bufferSize);
        else *c = '\0';
    }

    size_t len = strlen(buffer);
    if (len > 0 && strstr("\\/", &buffer[len - 1]))
    {
        buffer[len - 1] = '\0';
    }

    return buffer;
}

const char *GetFileSizeAsText(size_t bytes)
{
    local_variable char buffer[256];
    double sz = (double)bytes;
    char ex = 'B';

    if (sz > 1024.0) { sz /= 1024.0; ex = 'K'; }
    if (sz > 1024.0) { sz /= 1024.0; ex = 'M'; }
    if (sz > 1024.0) { sz /= 1024.0; ex = 'G'; }
    if (sz > 1024.0) { sz /= 1024.0; ex = 'T'; }
    if (sz > 1024.0) { sz /= 1024.0; ex = 'P'; }
    if (sz > 1024.0) { sz /= 1024.0; ex = 'E'; }
    if (sz > 1024.0) { sz /= 1024.0; ex = 'Z'; }
    if (sz > 1024.0) { sz /= 1024.0; ex = 'Y'; }

    if (sz < 0.01)
    {
        sprintf_s(buffer, sizeof(buffer), "        -");
    }
    else
    {
        sprintf_s(buffer, sizeof(buffer), "% 8.2lf%c", sz, ex);
    }

    return buffer;
}

const char *GetWorkingDirectory()
{
    local_variable char workingDir[MAX_PATH] = { 0 };
    GetCurrentDirectoryA(MAX_PATH, workingDir);
    return workingDir;
}
