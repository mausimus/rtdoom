#pragma once

#include "FrameBuffer.h"
#include "GameState.h"

namespace rtdoom
{
class Renderer
{
protected:
    const GameState& m_gameState;

    static bool IsVisible(int x, int y, FrameBuffer& frameBuffer);

    void DrawLine(int sx, int sy, int dx, int dy, int color, float lightness, FrameBuffer& frameBuffer) const;

public:
    Renderer(const GameState& gameState);
    virtual ~Renderer();

    virtual void RenderFrame(FrameBuffer& frameBuffer) = 0;
};
} // namespace rtdoom
