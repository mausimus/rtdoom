#pragma once

#include "Renderer.h"
#include "WADFile.h"
#include "Projection.h"
#include "Frame.h"
#include "Painter.h"

namespace rtdoom
{
class SoftwareRenderer : public Renderer
{
protected:
    struct VisibleSegment
    {
        VisibleSegment(const Segment& segment) : mapSegment {segment} { }

        const Segment& mapSegment;
        Vector         normalVector;
        int            startX;
        int            endX;
        float          startAngle;
        float          endAngle;
        float          normalOffset;
    };

    const float s_skyHeight = NAN;

    void Initialize(FrameBuffer& frameBuffer);
    void RenderSegments() const;
    void RenderPlanes() const;
    void RenderSprites() const;
    void RenderOverlay() const;
    void RenderMapSegment(const Segment& segment) const;
    void RenderMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const;
    void RenderSpriteThing(Frame::SpriteThing* const thing) const;
    void RenderSpriteWall(Frame::SpriteWall* const wall) const;

    std::vector<std::vector<bool>> ClipSprite(int startX, int startY, int spriteWidth, int spriteHeight, float spriteScale) const;
    Angle                          GetViewAngle(int x, const VisibleSegment& visibleSegment) const;

    FrameBuffer*                m_frameBuffer;
    const WADFile&              m_wadFile;
    std::unique_ptr<Projection> m_projection;
    std::unique_ptr<Frame>      m_frame;
    std::unique_ptr<Painter>    m_painter;
    RendererBase::RenderingMode m_renderingMode;

public:
    SoftwareRenderer(const GameState& gameState, const WADFile& wadFile);
    ~SoftwareRenderer();

    virtual void RenderFrame(FrameBuffer& frameBuffer) override;
    Frame*       GetLastFrame() const;
    void         SetMode(RendererBase::RenderingMode renderingMode);
};
} // namespace rtdoom
