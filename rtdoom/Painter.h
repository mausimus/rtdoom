#pragma once

#include "Frame.h"
#include "Projection.h"
#include "GameState.h"

namespace rtdoom
{
	class Painter
	{
	protected:
		FrameBuffer& m_frameBuffer;

	public:
		virtual void PaintWall(int x, const Frame::Span& span, const Frame::PainterContext& textureContext) const = 0;
		virtual void PaintSprite(int x, int sy, std::vector<bool> clipping, const Frame::PainterContext& textureContext) const = 0;
		virtual void PaintPlane(const Frame::Plane& plane) const = 0;

		Painter(FrameBuffer& frameBuffer);
		virtual ~Painter();
	};
}