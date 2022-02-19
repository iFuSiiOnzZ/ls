#include "win32.h"

#include <AccCtrl.h>
#include <AclAPI.h>

#include <stdlib.h>

#pragma comment(lib, "advapi32.lib")

// Check if handle is not NULL, close it and assign NULL to it
#define CHECK_CLOSE_HANDLE(x) do { if(x) { CloseHandle(x); x = NULL; } } while(0)

///////////////////////////////////////////////////////////////////////////////

const char *GetLastErrorAsString()
{

    DWORD errorMessageID = GetLastError();
    static char buffer[4096]; buffer[0] = '\0';

    if (errorMessageID == 0)
    {
        return "";
    }

    FormatMessageA
    (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer, sizeof(buffer), NULL
    );

    return buffer;
}

///////////////////////////////////////////////////////////////////////////////

void GetPermissions(const char *path, asset_t *asset)
{
    PSECURITY_DESCRIPTOR security = NULL;
    DWORD length = 0, genericAccessRights = 0;
    HANDLE hToken = NULL, hImpersonatedToken = NULL;

    asset->accessRights.read = FALSE;
    asset->accessRights.write = FALSE;
    asset->accessRights.execution = FALSE;

    BOOL oK = GetFileSecurityA(path, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, NULL, 0, &length);
    if (oK) goto clean_up;

    if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) security = malloc(length);
    if (!security) goto clean_up;

    oK = GetFileSecurityA(path, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, security, length, &length);
    if (!oK) goto clean_up;

    oK = OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ, &hToken);
    if (!oK) goto clean_up;

    oK = DuplicateToken(hToken, SecurityImpersonation, &hImpersonatedToken);
    if (!oK) goto clean_up;

    GENERIC_MAPPING mapping = { 0xFFFFFFFF };
    PRIVILEGE_SET privileges = { 0 };

    DWORD grantedAccess = 0, privilegesLength = sizeof(privileges);
    BOOL result = FALSE;

    mapping.GenericAll = FILE_ALL_ACCESS;
    mapping.GenericRead = FILE_GENERIC_READ;
    mapping.GenericWrite = FILE_GENERIC_WRITE;
    mapping.GenericExecute = FILE_GENERIC_EXECUTE;

    {
        genericAccessRights = FILE_GENERIC_READ;
        MapGenericMask(&genericAccessRights, &mapping);

        AccessCheck(security, hImpersonatedToken, genericAccessRights, &mapping, &privileges, &privilegesLength, &grantedAccess, &result);
        asset->accessRights.read = result == TRUE;
    }

    {
        genericAccessRights = FILE_GENERIC_WRITE;
        MapGenericMask(&genericAccessRights, &mapping);

        AccessCheck(security, hImpersonatedToken, genericAccessRights, &mapping, &privileges, &privilegesLength, &grantedAccess, &result);
        asset->accessRights.write = result == TRUE;
    }

    {
        genericAccessRights = FILE_GENERIC_EXECUTE;
        MapGenericMask(&genericAccessRights, &mapping);

        AccessCheck(security, hImpersonatedToken, genericAccessRights, &mapping, &privileges, &privilegesLength, &grantedAccess, &result);
        asset->accessRights.execution = result == TRUE;
    }

    clean_up:
    CHECK_DELETE(security);
    CHECK_CLOSE_HANDLE(hToken);
    CHECK_CLOSE_HANDLE(hImpersonatedToken);
}

BOOL GetOwnerAndDomain(const char *path, asset_t *asset)
{
    PSID pSidOwner = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;

    strcpy_s(asset->owner, OWNER_SIZE, "-");
    strcpy_s(asset->domain, DOMAIN_SIZE, "-");

    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) { GetLastErrorAsString(); return FALSE; }

    DWORD dwRtnCode = GetSecurityInfo(hFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pSidOwner, NULL, NULL, NULL, &pSD);
    if (dwRtnCode != ERROR_SUCCESS || pSidOwner == NULL) { GetLastErrorAsString(); CHECK_CLOSE_HANDLE(hFile); return FALSE; }

    SID_NAME_USE eUse = SidTypeUnknown; DWORD ownerSize = OWNER_SIZE, domainSize = DOMAIN_SIZE;
    BOOL result = LookupAccountSidA(NULL, pSidOwner, asset->owner, (LPDWORD)&ownerSize, asset->domain, (LPDWORD)&domainSize, &eUse);

    CHECK_CLOSE_HANDLE(hFile);
    return result;
}

///////////////////////////////////////////////////////////////////////////////

BOOL GetLinkTarget(const char *path, asset_t *asset)
{
    HANDLE h = CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) { return FALSE; }

    DWORD wBytes = GetFinalPathNameByHandleA(h, asset->link, MAX_PATH, VOLUME_NAME_DOS);
    CHECK_CLOSE_HANDLE(h);

    return wBytes < MAX_PATH;
}

///////////////////////////////////////////////////////////////////////////////

void TranslateAttributes(size_t attributes, asset_t *asset)
{
    asset->type.directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
    asset->type.document = !asset->type.directory;

    asset->type.compressed = (attributes & FILE_ATTRIBUTE_COMPRESSED) == FILE_ATTRIBUTE_COMPRESSED;
    asset->type.encrypted = (attributes & FILE_ATTRIBUTE_ENCRYPTED) == FILE_ATTRIBUTE_ENCRYPTED;

    asset->type.temporary = (attributes & FILE_ATTRIBUTE_TEMPORARY) == FILE_ATTRIBUTE_TEMPORARY;
    asset->type.system = (attributes & FILE_ATTRIBUTE_SYSTEM) == FILE_ATTRIBUTE_SYSTEM;

    asset->type.symlink = (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT;
    asset->type.hidden = (attributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN;

    // If name is set and starts with '.' or '$' market as hidden
    asset->type.hidden = (asset->name[0] == '.' || asset->name[0] == '$');
}

BOOL IsValidDirectory(const char *path)
{
    DWORD dwAttrib = GetFileAttributesA(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL IsValidDocument(const char *path)
{
    DWORD dwAttrib = GetFileAttributesA(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

size_t TranslateFileSize(WIN32_FIND_DATAA *fd)
{
    ULARGE_INTEGER ul;
    ul.HighPart = fd->nFileSizeHigh;
    ul.LowPart = fd->nFileSizeLow;
    return (size_t)ul.QuadPart;
}

///////////////////////////////////////////////////////////////////////////////

BOOL EnableVirtualTerminal()
{
    HANDLE handleOut = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD consoleMode = 0;
    GetConsoleMode(handleOut, &consoleMode);

    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    consoleMode |= DISABLE_NEWLINE_AUTO_RETURN;

    return SetConsoleMode(handleOut, consoleMode);
}

BOOL DisableVirtualTerminal()
{
    HANDLE handleOut = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD consoleMode = 0;
    GetConsoleMode(handleOut, &consoleMode);

    consoleMode |= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    consoleMode |= ~DISABLE_NEWLINE_AUTO_RETURN;

    return SetConsoleMode(handleOut, consoleMode);
}

///////////////////////////////////////////////////////////////////////////////

BOOL GetScreenBufferSize(size_t *width, size_t *height)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
    int ret = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    if (ret)
    {
        *width  = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        *height = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    }

    return ret;
}

BOOL GetCursorPosition(int *x, int *y)
{
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    int ret = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cbsi);

    if (ret)
    {
        *x = cbsi.dwCursorPosition.X;
        *y = cbsi.dwCursorPosition.Y;
    }

    return ret;

}

BOOL SetCursorPosition(int x, int y)
{
    COORD pos = { (SHORT)x, (SHORT)y };
    return SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

///////////////////////////////////////////////////////////////////////////////
