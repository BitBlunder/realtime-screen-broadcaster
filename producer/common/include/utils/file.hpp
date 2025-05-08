#ifndef FILE_HPP
#define FILE_HPP

namespace file
{
	bool
	path_clean(char* path);

	bool
	path_append(char* path, const char* append);

	bool
	file_base(char** path);

	bool
	file_copy(const char* src, const char* dst, bool fail_if_exists);
}

#endif