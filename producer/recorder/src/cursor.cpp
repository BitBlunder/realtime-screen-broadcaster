#include <cursor.hpp>

#include <string.h>

#include <capture.hpp>

CursorState*
cursor_init()
{
	CursorState* cursor_state = new CursorState;
	if (!cursor_state)
		return nullptr;

	cursor_state->visible = false;

	cursor_state->size = 0;
	cursor_state->shape = nullptr;

	cursor_state->pos.x = 0;
	cursor_state->pos.y = 0;

	return cursor_state;
}

void
cursor_blend(const CursorState *cs,
	uint8_t *dst_bgra, int dst_pitch, int frame_w, int frame_h)
{
	if (!cs->visible || !cs->shape)
		return;

	const int cw = cs->info.Width;
	const int ch = cs->info.Height;

	for (int y = 0; y < ch; ++y)
	{
		int dy = cs->pos.y + y;

		if (dy < 0 || dy >= frame_h)
			continue;

		const uint8_t* srow = cs->shape + y * cw * 4;
		uint8_t* drow = dst_bgra + dy * dst_pitch + cs->pos.x * 4;


		for (int x = 0; x < cw; ++x)
		{
			int dx = cs->pos.x + x;

			if (dx < 0 || dx >= frame_w)
				continue;

			const uint8_t *s = srow + x*4;

			uint8_t a = s[0];

			if (!a)
				continue;

			uint8_t *d = drow + x*4;

			d[0] = (s[3]*a + d[0]*(255-a)) >> 8;
			d[1] = (s[2]*a + d[1]*(255-a)) >> 8;
			d[2] = (s[1]*a + d[2]*(255-a)) >> 8;
			d[3] = 255;
		}
	}
}

void
cursor_update(CursorState *cs,
	IDXGIOutputDuplication *dup, const DXGI_OUTDUPL_FRAME_INFO *fi)
{
	if (fi->PointerShapeBufferSize)
	{
		free(cs->shape);
		cs->shape = (uint8_t*)malloc(fi->PointerShapeBufferSize);
		UINT req = 0;
		dup->GetFramePointerShape(fi->PointerShapeBufferSize,
								   cs->shape,&req,&cs->info);
		cs->size = fi->PointerShapeBufferSize;
	}
	cs->visible = fi->PointerPosition.Visible;
	cs->pos = fi->PointerPosition.Position;
}

void
cursor_free(CursorState *cs)
{
	free(cs->shape);
	memset(cs,0,sizeof *cs);
}