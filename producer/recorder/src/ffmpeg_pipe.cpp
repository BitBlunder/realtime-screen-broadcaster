#include <ffmpeg_pipe.hpp>

#include <vector>
#include <cstdio>

#include <windows.h>

#include <capture.hpp>

#include <utils/log.hpp>
#include <utils/win32_strings.hpp>
#include <utils/win32_symbols.hpp>

struct FFMPEGPipeContext
{
	const char* cmd;

	HANDLE h_std_in_read;
	HANDLE h_std_in_write;
	HANDLE h_std_out_read;
	HANDLE h_std_out_write;
};

static char*
_create_cmd(const FFMPEGPipeConfig& pipe_config)
{
	char* cmd = new char[256];
	if (!cmd)
		return nullptr;

	sprintf(cmd,
		"%s "
		"-f rawvideo -pix_fmt bgra -s 1920x1080 -framerate %d -i pipe:0 "   /* raw‐pipe input */
		"-vf \"scale=%d:%d\" "                                              /* scale filter */
		"-c:v mjpeg -q:v %d -pix_fmt yuvj420p -f mjpeg pipe:1",             /* MJPEG output */
		pipe_config.exe_path,
		pipe_config.frames_per_second,
		pipe_config.width, pipe_config.height,
		pipe_config.quality
	);

	LOG_INFO("executing command: %s", cmd);

	return cmd;
}

int
_create_pipe(FFMPEGPipeContext* self)
{
	SECURITY_ATTRIBUTES security_attributes {
		sizeof(security_attributes), NULL, TRUE };

	HANDLE h_std_in_read  = NULL;
	HANDLE h_std_in_write = NULL;
	if (!fptr_CreatePipe(&h_std_in_read, &h_std_in_write, &security_attributes, 0))
		return -1;

	if (!SetHandleInformation(h_std_in_write, HANDLE_FLAG_INHERIT, 0))
		return -1;

	HANDLE h_std_out_read  = NULL;
	HANDLE h_std_out_write = NULL;
	if (!fptr_CreatePipe(&h_std_out_read, &h_std_out_write, &security_attributes, 0))
		return -1;

	if (!SetHandleInformation(h_std_out_read, HANDLE_FLAG_INHERIT, 0))
		return -1;

	self->h_std_in_read = h_std_in_read;
	self->h_std_in_write = h_std_in_write;
	self->h_std_out_read = h_std_out_read;
	self->h_std_out_write = h_std_out_write;

	return 0;
}

int
_create_process(FFMPEGPipeContext* self)
{
	STARTUPINFO ffmpeg_startup_info;
	PROCESS_INFORMATION ffmpeg_process_info;

	ZeroMemory(&ffmpeg_startup_info, sizeof(STARTUPINFO));

	ffmpeg_startup_info.cb = sizeof(STARTUPINFO);

	ffmpeg_startup_info.hStdInput = self->h_std_in_read;
	ffmpeg_startup_info.hStdError = NULL;
	ffmpeg_startup_info.hStdOutput = self->h_std_out_write;

	ffmpeg_startup_info.dwFlags |= STARTF_USESTDHANDLES;
	ffmpeg_startup_info.dwFlags |= STARTF_USESHOWWINDOW;

	ffmpeg_startup_info.wShowWindow = SW_HIDE;

	if (!fptr_CreateProcessA(
		NULL,                 // name of the module to be executed
		self->cmd,            // command line to be executed
		NULL,                 // process security attributes
		NULL,                 // primary thread security attributes
		TRUE,                 // handles are inherited
		NULL,                 // flags that control the priority class and the creation of the process
		NULL,                 // pointer to the environment block for the new process
		NULL,                 // full path to the current directory for the process
		&ffmpeg_startup_info, // STARTUPINFO pointer
		&ffmpeg_process_info  // receives identification information about the new process.
	)) {
		fptr_CloseHandle(self->h_std_in_read);
		fptr_CloseHandle(self->h_std_out_write);

		fptr_CloseHandle(ffmpeg_process_info.hThread);
		fptr_CloseHandle(ffmpeg_process_info.hProcess);

		return -1;
	}

	fptr_CloseHandle(self->h_std_in_read);
	fptr_CloseHandle(self->h_std_out_write);

	fptr_CloseHandle(ffmpeg_process_info.hThread);
	fptr_CloseHandle(ffmpeg_process_info.hProcess);

	return 0;
}


FFMPEGPipeContext*
ffmpeg_pipe_init(const FFMPEGPipeConfig& pipe_config)
{
	FFMPEGPipeContext* self = new FFMPEGPipeContext;
	if (!self) {
		return nullptr;
	}

	self->cmd = _create_cmd(pipe_config);
	if (!self->cmd) {
		delete self;
		return nullptr;
	}

	if (_create_pipe(self) != 0) {
		delete self;
		return nullptr;
	}

	if (_create_process(self) != 0) {
		delete self;
		return nullptr;
	}

	return self;
}

int
pipe_ffmpeg_read(FFMPEGPipeContext* self, uint8_t** data, size_t* size)
{
	static std::vector<uint8_t> buffer;

	constexpr size_t CHUNK = 32 * 1024;
	uint8_t tmp[CHUNK];

	DWORD bytes_read;
	while (true)
	{
		if (!fptr_ReadFile(self->h_std_out_read, tmp, CHUNK, &bytes_read, NULL) || bytes_read == 0)
			return -1;

		buffer.insert(buffer.end(), tmp, tmp + bytes_read);

		size_t soi = 0;
		for (size_t i = 1; i < buffer.size(); i++)
		{
			if (buffer[i - 1] == 0xFF && buffer[i] == 0xD8)
				soi = i - 1;

			if (soi && buffer[i - 1] == 0xFF && buffer[i] == 0xD9)
			{
				size_t frame_length = i - soi + 1;

				uint8_t* jpg_data = new uint8_t[frame_length];
				memcpy(jpg_data, &buffer[soi], frame_length);

				*data = jpg_data;
				*size = frame_length;

				buffer.erase(buffer.begin(), buffer.begin() + i + 1);

				return 0;
			}
		}
	}
}

int
pipe_ffmpeg_write(FFMPEGPipeContext* self, const CaptureFrame* frame)
{
	for (int y = 0; y < frame->height; y++)
	{
		const uint8_t* row = frame->data + (y * frame->pitch);

		const uint8_t* ptr = row;
		int bytes_remaning = frame->pitch;

		while (bytes_remaning)
		{
			DWORD bytes_written = 0;

			if (!fptr_WriteFile(self->h_std_in_write, ptr, bytes_remaning, &bytes_written, NULL))
				return -1;

			ptr += bytes_written;
			bytes_remaning -= bytes_written;
		}
	}

	return 0;
}

void
pipe_ffmpeg_close_read_handle(FFMPEGPipeContext* self)
{
	fptr_CloseHandle(self->h_std_out_read);
}

void
pipe_ffmpeg_close_write_handle(FFMPEGPipeContext* self)
{
	fptr_CloseHandle(self->h_std_in_write);
}

void
pipe_ffmpeg_free(FFMPEGPipeContext* self)
{
	if (!self)
		return;

	free(self);
}
