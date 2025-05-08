#ifndef CAPTURE_HPP
#define CAPTURE_HPP

#include <cstdint>

struct CaptureContext;

struct CaptureFrame
{
	uint8_t* data;

	int pitch;
	int width, height;

	int64_t pts_us;
};

struct CaptureConfig
{
	int monitor_index = 0;
};

CaptureContext*
capture_init(const CaptureConfig* capture_config);

int
capture_step(CaptureContext* capture_context, CaptureFrame* dst);

void
capture_free(CaptureContext* capture_context);

#endif