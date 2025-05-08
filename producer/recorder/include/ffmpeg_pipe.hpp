#ifndef PIPE_FFMPEG_HPP
#define PIPE_FFMPEG_HPP

#include <cstdint>

struct CaptureFrame;
struct FFMPEGPipeContext;

struct FFMPEGPipeConfig
{
	char exe_path [128];

	size_t width;
	size_t height;

	uint8_t quality;
	uint8_t frames_per_second;
};

FFMPEGPipeContext*
ffmpeg_pipe_init(const FFMPEGPipeConfig& pipe_config);

int
pipe_ffmpeg_read(FFMPEGPipeContext* self, uint8_t** data, size_t* size);

int
pipe_ffmpeg_write(FFMPEGPipeContext* self, const CaptureFrame* frame);

void
pipe_ffmpeg_close_read_handle(FFMPEGPipeContext* self);

void
pipe_ffmpeg_close_write_handle(FFMPEGPipeContext* self);

void
pipe_ffmpeg_free(FFMPEGPipeContext* self);

#endif
