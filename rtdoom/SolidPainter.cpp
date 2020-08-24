#include "pch.h"
#include "SolidPainter.h"
#include "Projection.h"
#include "MathCache.h"

namespace rtdoom
{
SolidPainter::SolidPainter(FrameBuffer& frameBuffer, const Projection& projection) : Painter {frameBuffer}, m_projection {projection} {}

void SolidPainter::PaintWall(int x, const Frame::Span& span, const Frame::PainterContext& textureContext) const
{
    if(textureContext.textureName != "-")
    {
        m_frameBuffer.VerticalLine(x, span.s, span.e, s_wallColor, textureContext.lightness);
    }
}

void SolidPainter::PaintSprite(int /*x*/,
                               int /*sy*/,
                               std::vector<bool> /*occlusion*/,
                               const Frame::PainterContext& /*textureContext*/) const
{}

void SolidPainter::PaintPlane(const Frame::Plane& plane) const
{
    const bool isSky = plane.isSky();

    for(size_t y = 0; y < plane.spans.size(); y++)
    {
        const auto& spans          = plane.spans[y];
        auto        centerDistance = m_projection.PlaneDistance(y, plane.h);
        const float lightness      = isSky ? 1 : m_projection.Lightness(centerDistance) * plane.lightLevel;

        for(auto span : spans)
        {
            const auto sx = std::max(0, span.s);
            const auto ex = std::min(m_frameBuffer.m_width - 1, span.e);
            const auto nx = ex - sx + 1;

            m_frameBuffer.HorizontalLine(sx, ex, y, s_planeColor, lightness);
        }
    }
}

SolidPainter::~SolidPainter() {}
} // namespace rtdoom
