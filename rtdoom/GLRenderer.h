#pragma once

#include <SDL.h>
#include "Renderer.h"
#include "WADFile.h"
#include "Projection.h"
#include "Frame.h"
#include "Painter.h"
#include "GLContext.h"

namespace rtdoom
{
class GLRenderer : public Renderer
{
protected:
    int m_width;
    int m_height;

    void RenderSegments() const;
    void RenderOverlay() const;

    GLContext m_context;

public:
    GLRenderer(const GameState& gameState, const WADFile& wadFile, int width, int height);
    ~GLRenderer();

    void Initialize();
    void Resize(int width, int height);
    void LoadMap();
    void Reset();

    virtual void RenderFrame();
    virtual void RenderFrame(FrameBuffer& frameBuffer) override;
};
} // namespace rtdoom
