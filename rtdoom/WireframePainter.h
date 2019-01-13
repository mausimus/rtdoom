#pragma once

#include "Painter.h"

namespace rtdoom
{
	class WireframePainter : public Painter
	{
	protected:
		const int s_wallColor = 1;

	public:
		void PaintWall(int x, const Frame::Span& span, const Frame::PainterContext& textureContext) const override;
		void PaintSprite(int x, int sy, std::vector<bool> occlusion, const Frame::PainterContext& textureContext) const override;
		void PaintPlane(const Frame::Plane& plane) const override;

		WireframePainter(FrameBuffer& frameBuffer);
		~WireframePainter();
	};
}