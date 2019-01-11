#include "pch.h"
#include "SolidPainter.h"
#include "Projection.h"
#include "MathCache.h"

namespace rtdoom
{
	SolidPainter::SolidPainter(FrameBuffer& frameBuffer, const Projection& projection)
		: Painter{ frameBuffer }, m_projection{ projection }
	{
	}

	void SolidPainter::PaintWall(int x, const Frame::Span& span, const Frame::PainterContext& textureContext) const
	{
		if (textureContext.textureName != "-")
		{
			m_frameBuffer.VerticalLine(x, span.s, span.e, s_wallColor, textureContext.lightness);
		}
	}

	void SolidPainter::PaintSprite(int x, int sy, std::vector<bool> clipping, const Frame::PainterContext & textureContext) const
	{
	}

	void SolidPainter::PaintPlane(const Frame::Plane& plane) const
	{
		const bool isSky = plane.isSky();

		for (size_t y = 0; y < plane.spans.size(); y++)
		{
			const auto& spans = plane.spans[y];
			auto centerDistance = m_projection.PlaneDistance(y, plane.h);
			const float lightness = isSky ? 1 : m_projection.Lightness(centerDistance) * plane.lightLevel;

			for (auto span : spans)
			{
				const auto sx = std::max(0, span.s);
				const auto ex = std::min(m_frameBuffer.m_width - 1, span.e);
				const auto nx = ex - sx + 1;

				std::vector<int> texels(nx);
				for (auto x = sx; x <= ex; x++)
				{
					auto viewAngle = m_projection.ViewAngle(x);
					const auto distance = centerDistance / MathCache::instance().Cos(viewAngle);
					if (isSky)
					{
						m_frameBuffer.VerticalLine(x, y, y, s_planeColor, 1);
					}
					else
					{
						m_frameBuffer.VerticalLine(x, y, y, s_planeColor, m_projection.Lightness(distance) * plane.lightLevel);
					}
				}
			}
		}
	}

	SolidPainter::~SolidPainter()
	{
	}
}