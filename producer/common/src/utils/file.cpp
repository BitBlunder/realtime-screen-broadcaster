#include "../../include/utils/file.hpp"

#include <windows.h>


#include "../../include/utils/win32_strings.hpp"
#include "../../include/utils/win32_symbols.hpp"

namespace file
{
	bool
	path_clean(char* path)
	{
		if (!path || !*path)
			return false;

		size_t len = strlen(path);

		while (len && (path[len - 1] == '\\' || path[len - 1] == '/'))
			path[--len] = '\0';

		char *last_fwd = strrchr(path, '/');
		char *last_bck = strrchr(path, '\\');

		char *last_sep  = (last_bck > last_fwd) ? last_bck : last_fwd;
		if (!last_sep)
			return false;

		*last_sep = '\0';

		return true;
	}

	bool
	path_append(char* path, const char* append)
	{
		size_t path_len = strlen(path);
		size_t append_len = strlen(append);

		if (path_len + append_len + 2 > MAX_PATH)
			return false;

		if (path_len > 0 && path[path_len - 1] != '\\' && path[path_len - 1] != '/')
			strcat(path, "\\");

		strcat(path, append);

		return true;
	}

	bool
	file_base(char** path)
	{
		size_t path_len = strlen(*path);

		if (path_len > MAX_PATH)
			return false;

		*path += path_len - 1;
		while (**path != '\\' && **path != '/')
			(*path)--;

		(*path)++;

		return true;
	}

	bool
	file_copy(const char* src, const char* dst, bool fail_if_exists)
	{
		DWORD attr = fptr_GetFileAttributesA(dst);
		if (fail_if_exists && attr != INVALID_FILE_ATTRIBUTES)
			return false;

		DWORD creation_disposition = fail_if_exists ? CREATE_NEW : CREATE_ALWAYS;

		HANDLE h_src = fptr_CreateFileA(src, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (h_src == INVALID_HANDLE_VALUE)
			return false;

		HANDLE h_dst = fptr_CreateFileA(dst, GENERIC_WRITE, 0, NULL, creation_disposition, 0, NULL);
		if (h_dst == INVALID_HANDLE_VALUE)
		{
			fptr_CloseHandle(h_src);
			return false;
		}

		BYTE buffer[4096];
		DWORD bytes_read, bytes_written;

		while (fptr_ReadFile(h_src, buffer, sizeof(buffer), &bytes_read, NULL) && bytes_read > 0)
		{
			fptr_WriteFile(h_dst, buffer, bytes_read, &bytes_written, NULL);
			if (bytes_written != bytes_read)
			{
				fptr_CloseHandle(h_src);
				fptr_CloseHandle(h_dst);
				return false;
			}
		}

		fptr_CloseHandle(h_src);
		fptr_CloseHandle(h_dst);

		return true;
	}
}