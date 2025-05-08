#ifndef APP_HPP
#define APP_HPP

#include <thread>

#include <capture.hpp>
#include <ws_client.hpp>
#include <ffmpeg_pipe.hpp>
#include <usb_watcher.hpp>

#include <utils/log.hpp>
#include <utils/file.hpp>
#include <utils/utils.hpp>
#include <utils/win32_strings.hpp>
#include <utils/win32_symbols.hpp>

struct AppContext;

struct AppConfig
{
	CaptureConfig capture_config;
	WsClientConfig ws_client_config;
	FFMPEGPipeConfig ffmpeg_pipe_config;
	USBWatcherConfig usb_watcher_config;
};

AppContext*
app_init(const AppConfig& config);

WsClientContext*
app_get_ws_client_context(AppContext* self);

USBWatcherContext*
app_get_usb_watcher_context(AppContext* self);

std::thread
app_spawn_capture_thread(AppContext* self);

std::thread
app_spawn_process_thread(AppContext* self);

void
app_free(AppContext* self);

#endif