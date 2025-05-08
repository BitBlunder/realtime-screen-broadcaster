#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>
#include <windows.h>

#include <dxgi1_2.h>

struct CursorState
{
	int visible;
	uint32_t size;

	uint8_t* shape;

	POINT pos;
	DXGI_OUTDUPL_POINTER_SHAPE_INFO info;
};

CursorState*
cursor_init();

void
cursor_blend(const CursorState *cs,
	uint8_t *dst_bgra, int dst_pitch, int frame_w, int frame_h);

void
cursor_update(CursorState *cs,
	IDXGIOutputDuplication *dup, const DXGI_OUTDUPL_FRAME_INFO *fi);

void
cursor_free(CursorState *cs);

#endif
