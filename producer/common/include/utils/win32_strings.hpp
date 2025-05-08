#ifndef WIN32_STRINGS_HPP
#define WIN32_STRINGS_HPP

#include "encryption.hpp"

/*          KERNEL32 Functions          */
ENC_STRING(es_kernel32, 0x55, "kernel32.dll")

// Module Operations
ENC_STRING(es_load_library, 0x55, "LoadLibraryA")

// Process Operations
ENC_STRING(es_createpipe, 0x55, "CreatePipe")
ENC_STRING(es_createprocess, 0x55, "CreateProcessA")

// File Operations
ENC_STRING(es_readfile, 0x55, "ReadFile")
ENC_STRING(es_writefile, 0x55, "WriteFile")
ENC_STRING(es_createfilea, 0x55, "CreateFileA")

// Filesystem Operations
ENC_STRING(es_createdirectorya, 0x55, "CreateDirectoryA")
ENC_STRING(es_setcurrentdirectorya, 0x55, "SetCurrentDirectoryA")

// System and File Retrievers
ENC_STRING(es_getfileattributesa, 0x55, "GetFileAttributesA")
ENC_STRING(es_getmodulefilenamea, 0x55, "GetModuleFileNameA")
ENC_STRING(es_getenvironmentvariablea, 0x55, "GetEnvironmentVariableA")

// Clean Up Operations
ENC_STRING(es_closehandle, 0x55, "CloseHandle")


/*          SHELL32 Functions          */
ENC_STRING(es_shell32, 0x55, "shell32.dll")
// System Operations
ENC_STRING(es_shellexecutea, 0x55, "ShellExecuteA")

#endif