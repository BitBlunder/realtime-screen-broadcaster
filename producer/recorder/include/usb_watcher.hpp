#ifndef USB_WATCHER_HPP
#define USB_WATCHER_HPP

#define USB_STR_LEN 128

struct USBWatcherContext;

struct USBWatcherConfig
{
	char vid [USB_STR_LEN];
	char pid [USB_STR_LEN];
	char serial [USB_STR_LEN];
};

using USBWatcherCallback = void (*)(const char* device_path, void* user_context);

USBWatcherContext*
usb_watcher_init(const USBWatcherConfig& config);

void
usb_watcher_set_callback(USBWatcherContext* self, USBWatcherCallback callback, void* user_context);

void
usb_watcher_free(USBWatcherContext* self);

#endif