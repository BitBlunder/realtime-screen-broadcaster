#include "../../include/utils/log.hpp"

#include <cstdio>
#include <chrono>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <windows.h>


#include <utils/file.hpp>

#define COL_TIME  8
#define COL_LEVEL 5
#define COL_FILE  15
#define COL_LINE  5


namespace logger
{
	FILE* g_logfile_fd = nullptr;
	const char* g_logfile_path = nullptr;

	bool g_is_console_attached = false;

	static void
	_get_time_string(char* buffer, size_t size)
	{
		using clock = std::chrono::system_clock;

		auto chrono_time = clock::now();
		auto c_style_time = clock::to_time_t(chrono_time);
		auto calendar_time = localtime(&c_style_time);

		std::strftime(buffer, size, "%H:%M:%S", calendar_time);
	}

	static void
	_get_log_level_string(char* buffer, size_t size, int level)
	{
		static const char* strs[] = { "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
		sprintf(buffer, "%s", (level >= 0 && level <= 4) ? strs[level] : "UNK");
	}


	void
	logger_init(const char* logfile_path)
	{
	#if LOG_ENABLED
		#if LOG_ATTACH_CONSOLE
			AllocConsole();

			FILE* fp;
			freopen_s(&fp, "CONIN$",  "r", stdin);
			freopen_s(&fp, "CONOUT$", "w", stderr);
			freopen_s(&fp, "CONOUT$", "w", stdout);

			setvbuf(stdin,  nullptr, _IONBF, 0);
			setvbuf(stderr, nullptr, _IONBF, 0);
			setvbuf(stdout, nullptr, _IONBF, 0);

			g_is_console_attached = true;
		#endif

		g_logfile_path = logfile_path;
		if (g_logfile_path)
			g_logfile_fd = fopen(g_logfile_path, "a");

		if (!g_logfile_fd)
			LOG_ERROR("%s", strerror(errno));

		LOG_INFO("Logger initialised");
	#else
		(void)g_logfile_path;

		AllocConsole();

		HWND h_console = GetConsoleWindow();
		if (h_console != NULL)
			ShowWindow(h_console, SW_HIDE);

		freopen("CONIN$",  "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	#endif
	}

	void
	logger_free()
	{
	#if LOG_ENABLED
		LOG_INFO("Logger shutting down");

		if (g_logfile_fd)
		{
			fflush(g_logfile_fd);
			fclose(g_logfile_fd);

			g_logfile_fd = nullptr;
		}

		#if LOG_ATTACH_CONSOLE
			if (g_is_console_attached)
			{
				system("pause");

				fflush(stdout);

				fclose(stdin);
				fclose(stderr);
				fclose(stdout);

				FreeConsole();

				g_is_console_attached = false;
			}
		#endif
	#endif
	}

	void
	logger_print(int level, const char* file, int line, const char* fmt, ...)
	{
	#if LOG_ENABLED
		char time_str[32];
		_get_time_string(time_str, sizeof(time_str));

		char level_str[32];
		_get_log_level_string(level_str, sizeof(level_str), level);

		char msg[2048];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(msg, sizeof(msg), fmt, ap);
		va_end(ap);

		file::file_base(const_cast<char**>(&file));

		char linebuf[2304];
		snprintf(linebuf, sizeof linebuf,
			"%-*s | %-*s | %-*s:%0*d | %s\n",
			COL_TIME, time_str, COL_LEVEL, level_str, COL_FILE, file, COL_LINE, line, msg
		);

		fputs(linebuf, stdout);

		if (g_logfile_fd)
		{
			fputs(linebuf, g_logfile_fd);
			fflush(g_logfile_fd);
		}
	#else
		(void)level; (void)file; (void)line; (void)fmt;
	#endif
	}
}