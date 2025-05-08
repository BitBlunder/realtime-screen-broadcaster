#include <app.hpp>

void
on_usb_insertion(const char* path, void* user_context)
{
	auto ctx = static_cast<AppContext*>(user_context);
	ws_request_stop(app_get_ws_client_context(ctx));
	PostQuitMessage(0);
}

int
win32_load_configuration_file(AppConfig* config)
{
	char ip_buffer[32] = { 0 };

	GetPrivateProfileStringA("websocket", "ip", "",
		ip_buffer, sizeof(ip_buffer), ".\\config.ini");

	if (snprintf(config->ws_client_config.url, sizeof(config->ws_client_config.url), "ws://%s:80/ws", ip_buffer) < 0)
		return -1;

	GetPrivateProfileStringA("usb", "vid", "",
		config->usb_watcher_config.vid, sizeof(config->usb_watcher_config.vid), ".\\config.ini");
	GetPrivateProfileStringA("usb", "pid", "",
		config->usb_watcher_config.pid, sizeof(config->usb_watcher_config.vid), ".\\config.ini");
	GetPrivateProfileStringA("usb", "serial", "",
		config->usb_watcher_config.serial, sizeof(config->usb_watcher_config.vid), ".\\config.ini");

	GetPrivateProfileStringA("ffmpeg", "exe_path", "ffmpeg\\ffmpeg.exe",
		config->ffmpeg_pipe_config.exe_path, sizeof(config->ffmpeg_pipe_config.exe_path), ".\\config.ini");

	config->ffmpeg_pipe_config.width = GetPrivateProfileIntA("ffmpeg", "width",  1920, ".\\config.ini");
	config->ffmpeg_pipe_config.height = GetPrivateProfileIntA("ffmpeg", "height", 1080, ".\\config.ini");
	config->ffmpeg_pipe_config.quality = GetPrivateProfileIntA("ffmpeg", "quality",15, ".\\config.ini");
	config->ffmpeg_pipe_config.frames_per_second = GetPrivateProfileIntA("ffmpeg", "fps", 30, ".\\config.ini");

	return 0;
}

int
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	logger::logger_init("recorder_logs.txt");
	defer { logger::logger_free(); };

	utilities::win32_load_module_functions();
	utilities::win32_set_shell_to_current_directory();

	AppConfig app_configuarion;
	win32_load_configuration_file(&app_configuarion);

	AppContext* ctx = app_init(app_configuarion);
	if (!ctx) {
		LOG_FATAL("App initilization failed with %s", utilities::win32_get_error_string().c_str());
		return -1;
	}
	defer { app_free(ctx); };

	usb_watcher_set_callback(app_get_usb_watcher_context(ctx), &on_usb_insertion, ctx);

	std::thread writer_thread = app_spawn_capture_thread(ctx);
	std::thread reader_thread = app_spawn_process_thread(ctx);

	MSG msg;
	BOOL res;
	while ((res = PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) != 0 ||
			!ws_stop_requested(app_get_ws_client_context(ctx)))
		DispatchMessage(&msg);

	if (reader_thread.joinable())
		reader_thread.join();

	if (writer_thread.joinable())
		writer_thread.join();

	return 0;
}