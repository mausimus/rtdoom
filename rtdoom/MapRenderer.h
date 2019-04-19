#pragma once

#include "Renderer.h"

namespace rtdoom
{
class MapRenderer : public Renderer
{
protected:
    const float s_mapScale = 5.0f;

    void DrawMapLine(const Vertex& startVertex, const Vertex& endVertex, float lightness, FrameBuffer& frameBuffer) const;
    void MapToScreenCoords(float mx, float my, int& sx, int& sy, FrameBuffer& frameBuffer) const;

public:
    MapRenderer(const GameState& gameState);
    ~MapRenderer();

    virtual void RenderFrame(FrameBuffer& frameBuffer) override;
};
} // namespace rtdoom
