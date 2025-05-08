#include <capture.hpp>

#include <cstdio>
#include <cstdlib>

#include <windows.h>


#include <d3d11.h>
#include <dxgi1_2.h>

#include <cursor.hpp>

struct CaptureContext
{
	// D3D11 Interface
	ID3D11Device* device;         /* D3D11 device */
	ID3D11Texture2D* texture;     /* D3D11 CPU texture */
	ID3D11DeviceContext* context; /* D3D11 context */

	// DXGI Window Duplication API
	int width, height;                   /* Frame dimensions */
	IDXGIOutputDuplication* duplication; /* DXGI duplication interface */

	// GDI API fallback
	bool use_bitblt;       /* True if using BitBlt */
	HDC screen_dc;         /* Screen device context */
	HDC memory_dc;         /* Memory device context */
	uint8_t* dib_bits;     /* Pointer to DIB pixels */
	HBITMAP dib_bitmap;    /* DIB section */

	// Cursor overlay
	CursorState* cursor;

	// High-resolution timer
	LARGE_INTEGER qpc_base;
};

static int64_t qpc_us(const LARGE_INTEGER* base) {
	LARGE_INTEGER now, freq;
	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&freq);
	return (now.QuadPart - base->QuadPart) * 1000000 / freq.QuadPart;
}


static bool
_fallback_init(CaptureContext* capture_context)
{
	capture_context->use_bitblt = true;

	capture_context->screen_dc = GetDC(NULL);
	capture_context->memory_dc = CreateCompatibleDC(capture_context->screen_dc);

	if (!capture_context->memory_dc || !capture_context->screen_dc)
		return false;

	DEVMODE dm = {};
	dm.dmSize = sizeof(dm);
	if (EnumDisplaySettingsEx(NULL, ENUM_CURRENT_SETTINGS, &dm, 0)) {
		capture_context->width  = dm.dmPelsWidth;
		capture_context->height = dm.dmPelsHeight;
	}

	BITMAPINFO bi = {0};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = capture_context->width;
	bi.bmiHeader.biHeight      = -capture_context->height;
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	capture_context->dib_bitmap = CreateDIBSection(
		capture_context->screen_dc, &bi, DIB_RGB_COLORS, (void**)&capture_context->dib_bits, NULL, 0);

	if (!capture_context->dib_bitmap || !capture_context->dib_bits)
		return false;

	SelectObject(capture_context->memory_dc, capture_context->dib_bitmap);

	return true;
}

static void
_fallback_free(CaptureContext* capture_context)
{
	if (capture_context->dib_bitmap)
		DeleteObject(capture_context->dib_bitmap);

	if (capture_context->memory_dc)
		DeleteDC(capture_context->memory_dc);

	if (capture_context->screen_dc)
		ReleaseDC(NULL, capture_context->screen_dc);
}

static int
_fallback_capture(CaptureContext* capture_context, CaptureFrame* frame)
{
	bool result = BitBlt(
		capture_context->memory_dc,
		0, 0,
		capture_context->width,
		capture_context->height,
		capture_context->screen_dc,
		0, 0,
		SRCCOPY | CAPTUREBLT
	);

	if (!result)
		return - 1;

	frame->data = capture_context->dib_bits;
	frame->pitch = capture_context->width * 4;
	frame->width = capture_context->width;
	frame->height = capture_context->height;
	frame->pts_us = qpc_us(&capture_context->qpc_base);

	return 0;
}


static HRESULT
_create_d3d_device(ID3D11Device** out_device, ID3D11DeviceContext** out_context)
{
	D3D_FEATURE_LEVEL out_feature_level;
	HRESULT hr = D3D11CreateDevice(
		/* pAdapter:            */ NULL,                            // ← use the primary DXGI dxgi_adapter (NULL = “pick default hardware GPU”)
		/* DriverType:          */ D3D_DRIVER_TYPE_HARDWARE,        // ← create a hardware-accelerated device (vs. WARP or reference)
		/* SoftwareModule:      */ NULL,                            // ← only used if DriverType == D3D_DRIVER_TYPE_SOFTWARE
		/* Flags:               */ D3D11_CREATE_DEVICE_BGRA_SUPPORT,// ← enable BGRA texture support (for Direct2D interop)
		/* pFeatureLevels:      */ NULL,                            // ← array of feature‐levels to attempt (NULL = default list)
		/* FeatureLevelsCount:  */ 0,                               // ← length of that array (0 = use built-in descending list)
		/* SDKVersion:          */ D3D11_SDK_VERSION,               // ← ensure runtime matches headers version
		/* ppDevice:            */ out_device,                      // ← returned ID3D11Device**
		/* pFeatureLevel:       */ &out_feature_level,              // ← the one feature level actually chosen
		/* ppImmediateContext:  */ out_context                      // ← returned ID3D11DeviceContext** (immediate context)
	);

	return hr;
}

static HRESULT
_create_duplication_object(CaptureContext* capture_context, int monitor_index)
{
	IDXGIDevice* dxgi_device = NULL;    /* DXGI of the D3D11 device*/
	IDXGIAdapter* dxgi_adapter  = NULL; /* GPU that owns the D3D11 device */

	/* Convert ID3D11Device to IDXGIDevice to be able to use GetAdapter() */
	HRESULT hr = capture_context->device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi_device);
	if (FAILED(hr))
		return hr;

	/* Retrieve the adapter that created the device. This call adds a reference to the IDXGIAdapter. */
	dxgi_device->GetAdapter(&dxgi_adapter);
	dxgi_device->Release();

	/* Locate the desired monitor on the DXGI adapter. */
	IDXGIOutput* out = NULL;
	for (int i = 0; dxgi_adapter->EnumOutputs(i, &out) != DXGI_ERROR_NOT_FOUND; i++)
	{
		if (i == monitor_index)
			break;

		out->Release();
	}

	dxgi_adapter->Release();
	if (!out)
		return E_FAIL;

	/* DuplicateOutput lives on IDXGIOutput1 (DXGI 1.x), so upgrade the interface. */
	IDXGIOutput1* out1;
	hr = out->QueryInterface(__uuidof(IDXGIOutput1),(void**)&out1);
	out->Release();
	if (FAILED(hr))
		return hr;

	/* Create the desktop-duplication object. */
	hr = out1->DuplicateOutput(capture_context->device, &capture_context->duplication);
	out1->Release();

	return hr;
}

static int
_create_staging_texture(CaptureContext* capture_context)
{
	DXGI_OUTDUPL_DESC desc;
	capture_context->duplication->GetDesc(&desc);

	capture_context->width = desc.ModeDesc.Width;
	capture_context->height = desc.ModeDesc.Height;

	D3D11_TEXTURE2D_DESC td = {0};
	td.Width            = capture_context->width;
	td.Height           = capture_context->height;
	td.MipLevels        = 1;
	td.ArraySize        = 1;
	td.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage            = D3D11_USAGE_STAGING;
	td.CPUAccessFlags   = D3D11_CPU_ACCESS_READ;

	return FAILED(
		capture_context->device->CreateTexture2D(&td, NULL, &capture_context->texture)
	);
}


CaptureContext*
capture_init(const CaptureConfig* capture_config)
{
	CaptureContext* capture_context = new CaptureContext;
	if (!capture_context)
		return nullptr;

	QueryPerformanceCounter(&capture_context->qpc_base);

	HRESULT d3d_device_hr = _create_d3d_device(&capture_context->device, &capture_context->context);
	if (SUCCEEDED(d3d_device_hr))
	{
		HRESULT duplication_object_hr = _create_duplication_object(capture_context, capture_config->monitor_index);
		if (SUCCEEDED(duplication_object_hr) && !_create_staging_texture(capture_context))
		{
			capture_context->use_bitblt = false;
			capture_context->cursor = cursor_init();

			return capture_context;
		}
		if (duplication_object_hr == DXGI_ERROR_UNSUPPORTED)
		{
			_fallback_init(capture_context);
			return capture_context;
		}
	}

	capture_free(capture_context);
	return NULL;
}

int
capture_step(CaptureContext* capture_context, CaptureFrame* frame)
{
	if (capture_context->use_bitblt)
		return _fallback_capture(capture_context, frame);

	DXGI_OUTDUPL_FRAME_INFO fi;
	IDXGIResource* resource = NULL;
	HRESULT hr = capture_context->duplication->AcquireNextFrame(16, &fi, &resource);
	if (FAILED(hr)|| hr == DXGI_ERROR_WAIT_TIMEOUT)
		return 1;

	// cursor_update(capture_context->cursor, capture_context->duplication, &fi);

	ID3D11Texture2D* tex = NULL;
	resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);
	resource->Release();

	capture_context->context->CopyResource(capture_context->texture, tex);
	tex->Release();

	D3D11_MAPPED_SUBRESOURCE map;
	hr = capture_context->context->Map(capture_context->texture, 0, D3D11_MAP_READ, 0, &map);
	if (FAILED(hr)) {
		capture_context->duplication->ReleaseFrame();
		return -1;
	}

	frame->data      = (uint8_t*)map.pData;
	frame->pitch = map.RowPitch;
	frame->width     = capture_context->width;
	frame->height    = capture_context->height;
	frame->pts_us    = qpc_us(&capture_context->qpc_base);

	if (capture_context->cursor->visible)
	{
		cursor_blend(capture_context->cursor,
			frame->data, frame->pitch, capture_context->width, capture_context->height);
	}

	capture_context->context->Unmap(capture_context->texture, 0);
	capture_context->duplication->ReleaseFrame();

	return 0;
}

void
capture_free(CaptureContext* capture_context)
{
	if (!capture_context)
		return;

	if (capture_context->use_bitblt)
		_fallback_free(capture_context);
	else {
		cursor_free(capture_context->cursor);

		if (capture_context->texture)
			capture_context->texture->Release();

		if (capture_context->duplication)
			capture_context->duplication->Release();

		if (capture_context->context)
			capture_context->context->Release();

		if (capture_context->device)
			capture_context->device->Release();
	}

	free(capture_context);
}