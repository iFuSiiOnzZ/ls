#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define STARTUP_CONTAINER_SIZE  128  // startup capacity of the list container

#define PATH_SIZE MAX_PATH          // number of characters used for the path
#define DATE_SIZE       32          // number of characters used for the date

#define DOMAIN_SIZE 32              // number of characters used for the user domain (group)
#define OWNER_SIZE  32              // number of characters used for the user name (owner)

///////////////////////////////////////////////////////////////////////////////

// Check if pointer is not NULL, free it and assign NULL to it
#define CHECK_DELETE(x) do { if(x) { free(x); x = NULL; } } while(0)

// Compute the size of compile time array
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Enumerator indicating what kind of sorting should be applied.
 */
typedef enum sort_by_e
{
    SORT_NONE,
    SORT_DIRECTORY_FIRST,

    SORT_BY_SIZE,
    SORT_BY_NAME,

    SORT_BY_GROUP,
    SORT_BY_OWNER,

    SORT_BY_CREATION_DATE,
    SORT_BY_LAST_MODIFIED,
    SORT_BY_LAST_ACCESSED
} sort_by_e;

///////////////////////////////////////////////////////////////////////////////


/**
 * @brief Data structure containing the associated icon of a file extension
 * and it's color. The RGB color it is only used with virtual terminal.
 *
 * 'r'    : red color
 * 'g'    : green color
 * 'b'    : blue color
 *
 * 'ext'  : extension name, has to include the dot '.'
 * 'icons': UTF-8 string representation the icon
 */
typedef struct asset_metadata_t
{
    int r, g, b;
    const char *ext, *icon;
} asset_metadata_t;

/**
 * @brief User access rights for a given asset.
 *
 * 'read'       : read permissions      'r'
 * 'write'      : write permissions     'w'
 * 'execution'  : execution permissions 'x'
 */
typedef struct access_rights_t
{
    BOOL read;
    BOOL write;
    BOOL execution;
} access_rights_t;

/**
 * @brief Information about the asset type.
 * it could be a directory, document, symlink, etc
 */
typedef struct asset_type_t
{
    unsigned int directory : 1;
    unsigned int document : 1;

    unsigned int compressed : 1;
    unsigned int encrypted : 1;

    unsigned int temporary : 1;
    unsigned int system : 1;

    unsigned int symlink : 1;
    unsigned int hidden : 1;
} asset_type_t;

/**
 * @brief Information about the creation date,
 * last modification and write of the asset.
 *
 * 'access'         : last time it was accessed
 * 'creation'       : asset creation timestamp
 * 'modification'   : last time it was modify
 */
typedef struct timestamp_t
{
    unsigned long long access;
    unsigned long long creation;
    unsigned long long modification;
} timestamp_t;

/**
 * @brief It contains the main information of an asset.
 * By asset we understand document or directory.
 *
 * 'accessRights'   : see 'access_rights_t' structure
 * 'type'           : see 'asset_type_t' structure
 *
 * 'timestamp'      : FILETIME timestamp, creation, modification, based on sorting (by default creation)
 * 'size'           : size in bytes (only for files, directory don't have size)
 *
 * 'date'           : creation | accessed | modified date, based on sorting (by default creation)
 * 'name'           : name of the asset
 *
 * 'link'           : only for symlinks, contains the real path
 * 'path'           : full path to the asset
 *
 * 'domain'         : domain of the asset owner
 * 'owner'          : owner of the asset
 */
typedef struct asset_t
{
    const asset_metadata_t *metadata;

    access_rights_t accessRights;
    asset_type_t type;

    timestamp_t timestamp;
    size_t size;

    char date[DATE_SIZE];
    char name[PATH_SIZE];
    char link[PATH_SIZE];
    char path[PATH_SIZE];

    char domain[DOMAIN_SIZE];
    char owner[OWNER_SIZE];
} asset_t;

/**
 * @brief Data structure containing a list of assets inside a directory.
 *
 * 'capacity'   : amount of space available on the 'data' array
 * 'size'       : number of elements on the 'data' array
 * 'data'       : array with asset information
 */
typedef struct directory_t
{
    size_t size, capacity;
    asset_t data[0];
} directory_t;

/**
 * @brief Linked list that contains the directories to list.
 *
 * 'next'   pointer to the next directory to list
 * 'path'   current directory path to list
 */
typedef struct directory_list_t
{
    struct directory_list_t *next;
    char path[MAX_PATH];
} directory_list_t;

/**
 * @brief Contais information of the user input arguments.
 *
 * 'showAll'                : '-a', '--all'         show all assets found, '.', '..'  and hidden included
 * 'showAlmostAll'          : '-A', '--almost-all'  show all assets found, hidden included but not '.' and '..'
 * 'showLongFormat'         : '-l', '--long'        show extra information as size, creation date, permissions, etc
 *
 * 'reverseOrder'           : '-r', '--reverse'     reverse the sort order
 * 'recursiveList'          : '-R', '--recursive'   recursive list folders
 *
 * 'colors'                 :       '--colors'      colorize the output
 * 'showIcons'              :       '--icons'       show icons related to the asset
 *
 * 'showHelp'               : '-?', '--help'        show list of commands available
 * 'showVersion'            : '-v', '--version'     show current version number
 *
 * 'showMetaData'           :       '--smd'         display colors, icons and the file extensions
 * 'virtualTerminal'        :       '--virterm'     use virtual terminal for better color display
 *
 * 'sortField'              :                       which field is used to sort (name, size, owner, group, etc)
 * 'currentDir', 'lastDir'  :                       linked list of the directories to list
 */
typedef struct arguments_t
{
    BOOL showAll;
    BOOL showAlmostAll;
    BOOL showLongFormat;

    BOOL reverseOrder;
    BOOL recursiveList;

    BOOL colors;
    BOOL showIcons;

    BOOL showHelp;
    BOOL showVersion;

    BOOL showMetaData;
    BOOL virtualTerminal;

    sort_by_e sortField;
    directory_list_t *currentDir, *lastDir;
} arguments_t;

///////////////////////////////////////////////////////////////////////////////

static const asset_metadata_t g_AssetFullNameMetaData[] =
{
    // System predefined directory
    {230  ,  57  ,  70  ,             "windows"  ,               u8"\ue70f"}  ,  // 
    {168  , 218  , 220  ,               "users"  ,               u8"\uf74b"}  ,  // 
    {168  , 218  , 220  ,       "program files"  ,               u8"\uf756"}  ,  // 
    {168  , 218  , 220  , "program files (x86)"  ,               u8"\uf756"}  ,  // 

    // User predefined directory
    {168  , 218  , 220  ,            "contacts"  ,               u8"\ufbc9"}  ,  // ﯉
    {168  , 218  , 220  ,             "desktop"  ,               u8"\uf108"}  ,  // 
    {168  , 218  , 220  ,           "documents"  ,               u8"\uf752"}  ,  // 
    {168  , 218  , 220  ,           "downloads"  ,               u8"\uf498"}  ,  // 
    {168  , 218  , 220  ,           "favorites"  ,               u8"\ufb9b"}  ,  // ﮛ
    {168  , 218  , 220  ,               "links"  ,               u8"\uf0c1"}  ,  // 
    {168  , 218  , 220  ,               "music"  ,               u8"\uf883"}  ,  // 
    {168  , 218  , 220  ,              "videos"  ,               u8"\uf03d"}  ,  // 
    {168  , 218  , 220  ,            "pictures"  ,               u8"\uf74e"}  ,  // 
    {200  , 226  , 200  ,             "android"  ,               u8"\ue70e"}  ,  // 

    // Other type of folders
    {243  , 114  ,  44  ,                ".git"  ,               u8"\ue702"}  ,  // 
    {243  , 114  ,  44  ,          ".gitconfig"  ,               u8"\ue702"}  ,  // 
    {243  , 114  ,  44  ,          ".gitignore"  ,               u8"\ue702"}  ,  // 
    {243  , 114  ,  44  ,         ".gitmodules"  ,               u8"\ue702"}  ,  // 
    {243  , 114  ,  44  ,      ".gitattributes"  ,               u8"\ue702"}  ,  // 
    {254  , 197  , 187  ,             ".config"  ,               u8"\ue5fc"}  ,  // 
    {255  , 255  , 255  ,             ".vscode"  ,               u8"\ue70c"}  ,  // 
    {255  , 255  , 255  ,                 ".vs"  ,               u8"\ue70c"}  ,  // 
    {255  , 255  , 255  ,               ".atom"  ,               u8"\ue764"}  ,  // 
    {255  , 255  , 255  ,               ".idea"  ,               u8"\ue7b5"}  ,  // 

     // File names
    {255 ,  182  ,   0  ,          "license.md"  ,               u8"\ue60a"}  ,  // 
    {255 ,  182  ,   0  ,             "license"  ,               u8"\ue60a"}  ,  // 
    {255 ,  170  ,   0  ,           "readme.md"  ,               u8"\uf7fc"}  ,  // 
    {255 ,  170  ,   0  ,              "readme"  ,               u8"\uf7fc"}  ,  // 
    {255  , 158  ,   0  ,        "contributors"  ,               u8"\uf0c0"}  ,  // 
    {255  , 158  ,   0  ,     "contributors.md"  ,               u8"\uf0c0"}  ,  // 
    {255  , 145  ,   0  ,            "manifest"  ,               u8"\ue612"}  ,  // 
    {255  , 145  ,   0  ,         "manifest.md"  ,               u8"\ue612"}  ,  // 
    {255  , 133  ,   0  ,             "version"  ,               u8"\uf454"}  ,  // 
    {255  , 133  ,   0  ,          "version.md"  ,               u8"\uf454"}  ,  // 
    {255  , 121  ,   0  ,           "changelog"  ,               u8"\uf64f"}  ,  // 
    {255  , 121  ,   0  ,        "changelog.md"  ,               u8"\uf64f"}  ,  // 
    {122  , 139  , 142  ,         "jenkinsfile"  ,               u8"\ue767"}  ,  // 
    {  0  , 180  , 216  ,          "dockerfile"  ,               u8"\uf308"}  ,  // 
    {255  , 180  , 216  ,            "makefile"  ,               u8"\uf425"}  ,  // 
    {255  , 180  , 216  ,      "cmakelists.txt"  ,               u8"\uf425"}  ,  // 
};

static const asset_metadata_t g_AssetExtensionMetaData[] =
{
    // Windows executable and libraries
    {229  , 107  , 111  ,                ".exe"  ,               u8"\ufb13"}  ,  // ﬓ
    {181  , 101  , 118  ,                ".dll"  ,               u8"\uf1e1"}  ,  // 
    {249  , 132  ,  74  ,                ".sys"  ,               u8"\uf720"}  ,  // 
    {229  , 107  , 111  ,                ".bat"  ,               u8"\uf68c"}  ,  // 
    {229  , 107  , 111  ,                ".cmd"  ,               u8"\ue629"}  ,  // 
    {229  , 107  , 111  ,                ".com"  ,               u8"\ue629"}  ,  // 
    {229  , 107  , 111  ,                ".reg"  ,               u8"\ue629"}  ,  // 

    // Compress files
    {200  , 200  , 250  ,                 ".7z"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                 ".lz"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                 ".gz"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                 ".bz"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                ".lrz"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                ".zip"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                ".rar"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                ".tar"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                ".ace"  ,               u8"\uf410"}  ,  // 
    {200  , 200  , 250  ,                ".arc"  ,               u8"\uf410"}  ,  // 

    // Packaging files
    {200  , 226  , 200  ,                ".apk"  ,               u8"\ue70e"}  ,  // 
    {200  , 200  , 250  ,                ".xpi"  ,               u8"\uf487"}  ,  // 
    {200  , 200  , 250  ,                ".cab"  ,               u8"\uf487"}  ,  // 
    {200  , 200  , 250  ,                ".pak"  ,               u8"\uf487"}  ,  // 

    // Disk images
    {255  , 255  , 255  ,                ".iso"  ,               u8"\ue271"}  ,  // 
    {255  , 255  , 255  ,                ".dmg"  ,               u8"\ue271"}  ,  // 
    {255  , 255  , 255  ,                ".mdf"  ,               u8"\ue271"}  ,  // 
    {255  , 255  , 255  ,                ".nrg"  ,               u8"\ue271"}  ,  // 
    {255  , 255  , 255  ,                ".img"  ,               u8"\ue271"}  ,  // 
    {255  , 255  , 255  ,                ".dsk"  ,               u8"\ue271"}  ,  // 

    // Images
    {255  , 232  , 124  ,                ".ico"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".jpg"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,               ".jpeg"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".png"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".gif"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".bmp"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".svg"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,               ".webp"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".tif"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,               ".tiff"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".raw"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".tga"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                 ".ps"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,                ".pps"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,               ".ppsx"  ,               u8"\uf1c5"}  ,  // 
    {255  , 232  , 124  ,               ".jfif"  ,               u8"\uf1c5"}  ,  // 

     // Videos
    {237  , 145  ,  33  ,                ".mp4"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".m4v"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".mkv"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".avi"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".flv"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".flc"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".mov"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".wmv"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".ogv"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".ogm"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".ogx"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,                ".mpg"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,               ".mpeg"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,               ".webm"  ,               u8"\uf1c8"}  ,  // 
    {237  , 145  ,  33  ,               ".divx"  ,               u8"\uf1c8"}  ,  // 

     // Music
    {255  , 162  ,   0  ,                ".wav"  ,               u8"\uf722"}  ,  // 
    {255  , 162  ,   0  ,                ".mp3"  ,               u8"\uf722"}  ,  // 
    {255  , 162  ,   0  ,                ".wma"  ,               u8"\uf722"}  ,  // 
    {255  , 162  ,   0  ,                ".ogg"  ,               u8"\uf722"}  ,  // 
    {255  , 162  ,   0  ,                ".oga"  ,               u8"\uf722"}  ,  // 
    {255  , 162  ,   0  ,                ".aac"  ,               u8"\uf722"}  ,  // 
    {255  , 162  ,   0  ,               ".flac"  ,               u8"\uf722"}  ,  // 
    {255  , 162  ,   0  ,               ".midi"  ,               u8"\uf722"}  ,  // 

    // Text edit
    {255  , 255  , 255  ,                ".txt"  ,               u8"\uf0f6"}  ,  // 
    {255  , 100  , 100  ,                ".pdf"  ,               u8"\uf1c1"}  ,  // 
    {  3  , 131  , 135  ,                ".odt"  ,               u8"\uf1c2"}  ,  // 
    {  3  , 131  , 135  ,                ".doc"  ,               u8"\uf1c2"}  ,  // 
    {  3  , 131  , 135  ,               ".docx"  ,               u8"\uf1c2"}  ,  // 
    {  3  , 131  , 135  ,                ".ods"  ,               u8"\uf1c2"}  ,  // 
    {  3  , 131  , 135  ,                ".xls"  ,               u8"\uf1c3"}  ,  // 
    {  3  , 131  , 135  ,               ".xlsx"  ,               u8"\uf1c3"}  ,  // 
    {  3  , 131  , 135  ,               ".xlsm"  ,               u8"\uf1c3"}  ,  // 
    {  3  , 131  , 135  ,                ".odp"  ,               u8"\uf1c2"}  ,  // 
    {  3  , 131  , 135  ,                ".ppt"  ,               u8"\uf1c4"}  ,  // 
    {  3  , 131  , 135  ,               ".pptx"  ,               u8"\uf1c4"}  ,  // 

    // Simple text format
    {144  , 221  , 240  ,       ".editorconfig"  ,               u8"\ue615"}  ,  // 
    {144  , 221  , 240  ,                ".cfg"  ,               u8"\ue615"}  ,  // 
    {144  , 221  , 240  ,                ".ini"  ,               u8"\ue615"}  ,  // 
    { 39  , 125  , 161  ,               ".json"  ,               u8"\ue60b"}  ,  // 
    {249  , 132  ,  74  ,                ".xml"  ,               u8"\uf72d"}  ,  // 
    {239  , 217  , 206  ,                 ".md"  ,               u8"\uf853"}  ,  // 
    {166  , 117  , 161  ,                ".yml"  ,               u8"\ue009"}  ,  // 
    {166  , 117  , 161  ,               ".yaml"  ,               u8"\ue009"}  ,  // 

    // Fonts
    {144  , 190  , 109  ,                ".ttf"  ,               u8"\uf031"}  ,  // 
    {144  , 190  , 109  ,                ".otf"  ,               u8"\uf031"}  ,  // 
    {144  , 190  , 109  ,               ".font"  ,               u8"\uf031"}  ,  // 
    {144  , 190  , 109  ,               ".woff"  ,               u8"\uf031"}  ,  // 
    {144  , 190  , 109  ,              ".woff2"  ,               u8"\uf031"}  ,  // 

    // Programming
    {127  , 147  , 184  ,                  ".c"  ,               u8"\ue61e"}  ,  // 
    {127  , 147  , 184  ,                  ".h"  ,               u8"\ue61e"}  ,  // 
    {127  , 147  , 184  ,                 ".cc"  ,               u8"\ue61d"}  ,  // 
    {127  , 147  , 184  ,                ".cpp"  ,               u8"\ue61d"}  ,  // 
    {127  , 147  , 184  ,                ".inl"  ,               u8"\ue61d"}  ,  // 
    {127  , 147  , 184  ,                ".hpp"  ,               u8"\ue61d"}  ,  // 
    {255  , 155  ,  84  ,                ".asm"  ,               u8"\ufb32"}  ,  // גּ
    {212  , 106  , 106  ,                 ".cs"  ,               u8"\uf81a"}  ,  // 
    {212  , 106  , 106  ,                ".vba"  ,               u8"\ufb32"}  ,  // גּ
    {180  ,  89  , 122  ,                 ".sh"  ,               u8"\uf68c"}  ,  // 
    {180  ,  89  , 122  ,                ".zsh"  ,               u8"\uf68c"}  ,  // 
    {212  , 154  , 106  ,                 ".py"  ,               u8"\ue73c"}  ,  // 
    {255  , 255  , 255  ,                 ".go"  ,               u8"\ue626"}  ,  // 
    {255  , 255  , 255  ,                 ".rs"  ,               u8"\ue7a8"}  ,  // 
    {102  , 153  , 153  ,                ".lua"  ,               u8"\ue620"}  ,  // 
    {127  , 147  , 184  ,                ".php"  ,               u8"\ue73d"}  ,  // 
    {255  , 209  , 170  ,                ".jar"  ,               u8"\ue256"}  ,  // 
    {255  , 209  , 170  ,               ".java"  ,               u8"\ue256"}  ,  // 
    {255  , 209  , 170  ,             ".groovy"  ,               u8"\ue775"}  ,  // 
    {136  , 204  , 136  ,                ".css"  ,               u8"\ue74a"}  ,  // 
    {136  , 204  , 136  ,                ".htm"  ,               u8"\ue60e"}  ,  // 
    {136  , 204  , 136  ,               ".html"  ,               u8"\ue60e"}  ,  // 
    {255  , 209  , 170  ,             ".coffee"  ,               u8"\ue751"}  ,  // 
    {249  , 132  ,  74  ,              ".swift"  ,               u8"\ue755"}  ,  // 
    { 39  , 125  , 161  ,                 ".js"  ,               u8"\ue74e"}  ,  // 
    { 39  , 125  , 161  ,         ".javascript"  ,               u8"\ue74e"}  ,  // 

    // Data base
    {249  , 199  ,  79  ,                 ".db"  ,               u8"\uf1c0"}  ,  // 
    {249  , 199  ,  79  ,                ".sql"  ,               u8"\uf1c0"}  ,  // 
    {249  , 199  ,  79  ,               ".msql"  ,               u8"\uf1c0"}  ,  // 
    {249  , 199  ,  79  ,              ".mysql"  ,               u8"\uf1c0"}  ,  // 

    // SSL files
    { 59  , 145  , 181  ,                ".key"  ,               u8"\uf805"}  ,  // 
    { 59  , 145  , 181  ,                ".pem"  ,               u8"\uf805"}  ,  // 
    { 59  , 145  , 181  ,                ".crt"  ,               u8"\uf0a3"}  ,  // 

    // Build/Solution/Project files
    {255  , 255  , 255  ,               ".make"  ,               u8"\uf425"}  ,  // 
    {255  , 255  , 255  ,              ".cmake"  ,               u8"\uf425"}  ,  // 
    {254  , 228  ,  64  ,                ".sln"  ,               u8"\ue70c"}  ,  // 
    {175  , 123  , 249  ,             ".vcproj"  ,               u8"\ue70c"}  ,  // 
    {175  , 123  , 249  ,            ".vcxproj"  ,               u8"\ue70c"}  ,  // 
    {241  ,  91  , 181  ,            ".filters"  ,               u8"\uf0b0"}  ,  // 

    // Other type of files
    {249  , 199  ,  79  ,                 ".in"  ,               u8"\ufd40"}  ,  // ﵀
    {249  , 199  ,  79  ,                ".bin"  ,               u8"\uf471"}  ,  // 
    {249  , 199  ,  79  ,                ".dat"  ,               u8"\uf471"}  ,  // 
    {249  , 199  ,  79  ,                ".bak"  ,               u8"\ufb6f"}  ,  // ﭯ
    {249  , 199  ,  79  ,                ".tmp"  ,               u8"\uf43a"}  ,  // 
    {249  , 199  ,  79  ,                ".log"  ,               u8"\uf718"}  ,  // 
    {249  , 199  ,  79  ,               ".tlog"  ,               u8"\uf718"}  ,  // 
    {255  , 255  , 255  ,               ".part"  ,               u8"\uf43a"}  ,  // 
    {254  , 109  , 115  ,               ".lock"  ,               u8"\uf023"}  ,  // 
    {255  , 255  , 255  ,               ".path"  ,               u8"\uf440"}  ,  // 
    {249  , 199  ,  79  ,              ".cache"  ,               u8"\uf5e7"}  ,  // 
    {255  , 255  , 255  ,             ".backup"  ,               u8"\ufb6f"}  ,  // ﭯ
    {255  , 255  , 255  ,            ".torrent"  ,               u8"\uf661"}  ,  // 
};
