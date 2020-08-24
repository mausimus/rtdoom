#pragma once

#include "FrameBuffer.h"
#include "GameState.h"

namespace rtdoom
{
class RendererBase
{
protected:
    const GameState& m_gameState;

public:
    enum class RenderingMode
    {
        Wireframe,
        Solid,
        Textured,
        OpenGL
    };

    RendererBase(const GameState& gameState) : m_gameState {gameState} { }
    virtual ~RendererBase() { }
};

class Renderer : public RendererBase
{
protected:
    static bool IsVisible(int x, int y, FrameBuffer& frameBuffer);

    void DrawLine(int sx, int sy, int dx, int dy, int color, float lightness, FrameBuffer& frameBuffer) const;

public:
    virtual void RenderFrame(FrameBuffer& frameBuffer) = 0;

    Renderer(const GameState& gameState);
    virtual ~Renderer();
};
} // namespace rtdoom
