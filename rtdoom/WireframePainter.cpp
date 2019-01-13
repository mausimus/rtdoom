#include "pch.h"
#include "WireframePainter.h"
#include "Projection.h"
#include "MathCache.h"

namespace rtdoom
{
	WireframePainter::WireframePainter(FrameBuffer& frameBuffer)
		: Painter(frameBuffer)
	{
		m_frameBuffer.Clear();
	}

	void WireframePainter::PaintWall(int x, const Frame::Span& span, const Frame::PainterContext& textureContext) const
	{
		if (span.s > 0)
		{
			m_frameBuffer.VerticalLine(x, span.s, span.s, s_wallColor, textureContext.lightness);
		}
		if (span.e > 0)
		{
			m_frameBuffer.VerticalLine(x, span.e, span.e, s_wallColor, textureContext.lightness);
		}
		if (textureContext.textureName != "-" && textureContext.isEdge)
		{
			m_frameBuffer.VerticalLine(x, span.s + 1, span.e - 1, s_wallColor, textureContext.lightness / 2.0f);
		}
	}

	void WireframePainter::PaintSprite(int /*x*/, int /*sy*/, std::vector<bool> /*occlusion*/, const Frame::PainterContext& /*textureContext*/) const
	{
	}

	void WireframePainter::PaintPlane(const Frame::Plane& /*plane*/) const
	{
	}

	WireframePainter::~WireframePainter()
	{
	}
}