#pragma once

#include "Painter.h"

namespace rtdoom
{
class SolidPainter : public Painter
{
protected:
    const int s_wallColor  = 1;
    const int s_planeColor = 5;

    const Projection& m_projection;

public:
    void PaintWall(int x, const Frame::Span& span, const Frame::PainterContext& textureContext) const override;
    void PaintSprite(int x, int sy, std::vector<bool> occlusion, const Frame::PainterContext& textureContext) const override;
    void PaintPlane(const Frame::Plane& plane) const override;

    SolidPainter(FrameBuffer& frameBuffer, const Projection& projection);
    ~SolidPainter();
};
} // namespace rtdoom
