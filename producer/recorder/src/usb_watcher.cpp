#include <usb_watcher.hpp>

#include <string>

#include <windows.h>


#include <dbt.h>
#include <initguid.h>
#include <Usbiodef.h>

#include <utils/log.hpp>

struct USBWatcherContext
{
	void* user_context = nullptr;

	HWND h_window = nullptr;
	HINSTANCE h_module = nullptr;
	HDEVNOTIFY h_device = nullptr;

	USBWatcherConfig config;
	USBWatcherCallback callback = nullptr;
};

int
_matches(const USBWatcherConfig& id, const char* path)
{
	std::string std_str = path;

	if (std_str.find(id.vid)    != std::string::npos &&
		std_str.find(id.pid)    != std::string::npos &&
		std_str.find(id.serial) != std::string::npos
	)
		return 0;

	return -1;
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_NCCREATE)
	{
		auto* user_context = static_cast<USBWatcherContext*>(
			reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams
		);

		SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(user_context));
		return TRUE;
	}

	auto* user_context = reinterpret_cast<USBWatcherContext*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

	if (uMsg == WM_DEVICECHANGE && wParam == DBT_DEVICEARRIVAL && user_context)
	{
		auto* hdr = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);

		if (hdr && hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
		{
			auto* di = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(hdr);

			LOG_INFO("USB Device: %s has been inserted", di->dbcc_name);

			if (_matches(user_context->config, di->dbcc_name) == 0 && user_context->callback)
				user_context->callback(di->dbcc_name, user_context->user_context);
		}
	}

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

USBWatcherContext*
usb_watcher_init(const USBWatcherConfig& config)
{
	auto* self = new USBWatcherContext;
	self->config = config;
	self->h_module = GetModuleHandleW(nullptr);

	WNDCLASSA window_class {};
	window_class.hInstance = self->h_module;
	window_class.lpfnWndProc = WindowProcedure;
	window_class.lpszClassName = "UsbWatcherHiddenClass";

	if(!RegisterClassA(&window_class)) {
		delete self;
		return nullptr;
	}

	self->h_window = CreateWindowA(
		"UsbWatcherHiddenClass",
		NULL, NULL, NULL, NULL, NULL, NULL,
		HWND_MESSAGE, NULL, self->h_module, self);
	if (!self->h_window)
	{
		delete self;
		return nullptr;
	}

	DEV_BROADCAST_DEVICEINTERFACE_A f{};
	f.dbcc_size = sizeof(f);
	f.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	f.dbcc_classguid  = GUID_DEVINTERFACE_USB_DEVICE;

	self->h_device = RegisterDeviceNotificationA(self->h_window, &f, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (!self->h_device)
	{
		DestroyWindow(self->h_window);
		delete self;
		return nullptr;
	}

	return self;
}

void
usb_watcher_set_callback(USBWatcherContext* self, USBWatcherCallback callback, void* user_context)
{
	if (!self)
		return;

	self->callback = callback;
	self->user_context = user_context;
}

void
usb_watcher_free(USBWatcherContext* self)
{
	if (!self)
		return;

	if (self->h_device)
		UnregisterDeviceNotification(self->h_device);

	if (self->h_window)
		DestroyWindow(self->h_window);

	delete self;
}