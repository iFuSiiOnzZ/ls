#include "utils.h"
#include "win32.h"

#include <stdio.h>
#include <stdlib.h>

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

///////////////////////////////////////////////////////////////////////////////

void AddDirectoryToList(arguments_t *arguments, const char *path)
{
    directory_list_t *dir = malloc(sizeof(directory_list_t));
    if (dir == NULL) return;

    strncpy_s(dir->path, MAX_PATH, path, MAX_PATH);
    dir->next = NULL;

    if (arguments->lastDir == NULL)
    {
        arguments->lastDir = dir;
        arguments->currentDir = arguments->lastDir;
    }
    else
    {
        arguments->lastDir->next = dir;
        arguments->lastDir = dir;
    }
}

///////////////////////////////////////////////////////////////////////////////

char *ltrim(char *s)
{
    if (!s || s[0] == '\0')
    {
        return s;
    }

    size_t len = strlen(s);
    char *c = s;

    while (*c && isspace(*c))
    {
        ++c, --len;
    }

    if (s != c)
    {
        memmove(s, c, len + 1);
    }

    return s;
}

char *rtrim(char *s)
{
    if (!s || s[0] == '\0')
    {
        return s;
    }

    size_t len = strlen(s);
    char *c = s + len - 1;

    while (c != s && isspace(*c))
    {
        --c, --len;
    }

    c[isspace(*c) ? 0 : 1] = '\0';
    return s;
}

char *trim(char *s)
{
    return rtrim(ltrim(s));
}

///////////////////////////////////////////////////////////////////////////////

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

BOOL StringEndsWith(const char *str, const char *suffix)
{
    if (!str || !suffix) return FALSE;

    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);

    if (lensuffix > lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

const char *GetDirectoryFromPath(const char *path, char *buffer, size_t bufferSize)
{
    sprintf_s(buffer, bufferSize, "%s", path);

    if (IsValidDocument(path) || strstr(buffer, "*"))
    {
        char *c = (char *)FindLastDelimiter(buffer, "\\/");
        *c = '\0';
    }

    size_t len = strlen(buffer);
    if (buffer[len - 1] == '\\' || buffer[len - 1] == '/') buffer[len - 1] = '\0';

    return buffer;
}

///////////////////////////////////////////////////////////////////////////////

const char *GetFileSizeAsText(size_t bytes)
{
    double sz = (double)bytes;
    static char buffer[256];
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
    static char workingDir[MAX_PATH] = { 0 };
    GetCurrentDirectoryA(MAX_PATH, workingDir);
    return workingDir;
}

BOOL IsDotPath(const char *name)
{
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}
