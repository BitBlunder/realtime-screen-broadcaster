#ifndef LOG_HPP
#define LOG_HPP

namespace logger
{
	enum LogLevel
	{
		LOG_DBG = 0,
		LOG_INF,
		LOG_WRN,
		LOG_ERR,
		LOG_FTL
	};

	void
	logger_init(const char* logfile_path = nullptr);

	void
	logger_free();

	void
	logger_print(int level, const char* file, int line, const char* fmt, ...);
}

#ifndef LOG_LEVEL_THRESHOLD
	#define LOG_LEVEL_THRESHOLD 0
#endif

#if LOG_ENABLED
	#define LOG_DEBUG(fmt, ...)  do {                                                          \
			if (LOG_LEVEL_THRESHOLD <= logger::LOG_DBG)                                        \
				logger::logger_print(logger::LOG_DBG, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
		} while(0)

	#define LOG_INFO(fmt, ...)   do {                                                          \
			if (LOG_LEVEL_THRESHOLD <= logger::LOG_INF)                                        \
				logger::logger_print(logger::LOG_INF, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
		} while(0)

	#define LOG_WARN(fmt, ...)   do {                                                          \
			if (LOG_LEVEL_THRESHOLD <= logger::LOG_WRN)                                        \
				logger::logger_print(logger::LOG_WRN, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
		} while(0)

	#define LOG_ERROR(fmt, ...)  do {                                                          \
			if (LOG_LEVEL_THRESHOLD <= logger::LOG_ERR)                                        \
				logger::logger_print(logger::LOG_ERR, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
		} while(0)

	#define LOG_FATAL(fmt, ...)  do {                                                          \
			if (LOG_LEVEL_THRESHOLD <= logger::LOG_FTL)                                        \
				logger::logger_print(logger::LOG_FTL, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
		} while(0)
#else
	#define LOG_DEBUG(...) ((void)0)
	#define LOG_INFO(...)  ((void)0)
	#define LOG_WARN(...)  ((void)0)
	#define LOG_ERROR(...) ((void)0)
	#define LOG_FATAL(...) ((void)0)
#endif


#endif