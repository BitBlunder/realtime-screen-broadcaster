#include <app.hpp>

struct AppContext
{
	WsClientContext* ws_context;
	CaptureContext* capture_context;
	FFMPEGPipeContext* ffmpeg_pipe_context;
	USBWatcherContext* usb_watcher_context;
};

void
_capture_loop(AppContext* self)
{
	LOG_INFO("Recording screen...");

	CaptureFrame capture_frame;
	while (!ws_stop_requested(self->ws_context))
	{
		if (capture_step(self->capture_context, &capture_frame) < 0)
			LOG_ERROR("Failed to capture the current desktop frame");

		if (pipe_ffmpeg_write(self->ffmpeg_pipe_context, &capture_frame) < 0)
			LOG_ERROR("Failed to push the current desktop frame to the ffpmeg pipe:\n\t%s", utilities::win32_get_error_string().c_str());
	}

	LOG_INFO("Capture thread is closing...");

	pipe_ffmpeg_close_write_handle(self->ffmpeg_pipe_context);
}

void
_process_loop(AppContext* self)
{
	while (!ws_stop_requested(self->ws_context))
	{
		uint8_t* data; size_t len;
		if (pipe_ffmpeg_read(self->ffmpeg_pipe_context, &data, &len) == 0)
			ws_send_frame(self->ws_context, data, len);

		delete data;
	}

	LOG_INFO("Process thread is closing...");

	pipe_ffmpeg_close_read_handle(self->ffmpeg_pipe_context);
}


AppContext*
app_init(const AppConfig& config)
{
	AppContext* self = new AppContext;

	LOG_INFO("Initialising desktop capture context");
	self->capture_context = capture_init(&config.capture_config);
	if (!self->capture_context) {
		LOG_FATAL("desktop capture initilization failed with %s", utilities::win32_get_error_string().c_str());

		delete self;
		return nullptr;
	}

	LOG_INFO("Initialising ffmpeg pipe context");
	self->ffmpeg_pipe_context = ffmpeg_pipe_init(config.ffmpeg_pipe_config);
	if (!self->ffmpeg_pipe_context) {
		LOG_FATAL("ffmpeg pipe initilization failed with %s", utilities::win32_get_error_string().c_str());

		delete self;
		return nullptr;
	}

	LOG_INFO("Initialising USB watcher context");
	self->usb_watcher_context = usb_watcher_init(config.usb_watcher_config);
	if (!self->usb_watcher_context) {
		LOG_FATAL("USB watcher initilization failed with %s", utilities::win32_get_error_string().c_str());

		delete self;
		return nullptr;
	}

	LOG_INFO("Initialising websocket client context");
	self->ws_context = ws_init(config.ws_client_config);
	if (!self->ws_context) {
		LOG_FATAL("websocket initilization failed with %s", utilities::win32_get_error_string().c_str());

		delete self;
		return nullptr;
	}

	return self;
}

WsClientContext*
app_get_ws_client_context(AppContext* self)
{
	return self->ws_context;
}

USBWatcherContext*
app_get_usb_watcher_context(AppContext* self)
{
	return self->usb_watcher_context;
}

std::thread
app_spawn_capture_thread(AppContext* self)
{
	return std::thread(&_capture_loop, self);
}

std::thread
app_spawn_process_thread(AppContext* self)
{
	return std::thread(&_process_loop, self);
}

void
app_free(AppContext* self)
{
	LOG_INFO("Closing websocket context...");
	ws_free(self->ws_context);

	LOG_INFO("Closing ffmpeg pipe context...");
	pipe_ffmpeg_free(self->ffmpeg_pipe_context);

	LOG_INFO("Closing usb watcher context...");
	usb_watcher_free(self->usb_watcher_context);

	LOG_INFO("Closing desktop capture context...");
	capture_free(self->capture_context);
}